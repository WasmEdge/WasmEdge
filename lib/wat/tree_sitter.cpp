// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "tree_sitter.h"

#include <tree_sitter/api.h>

#include <algorithm>
#include <cstring>
#include <new>
#include <type_traits>
#include <utility>

namespace WasmEdge::WAT {

static_assert(sizeof(TSNode) <= sizeof(Node::Storage),
              "Node::Storage too small for TSNode");
static_assert(std::is_trivially_copyable_v<TSNode>,
              "TSNode must be trivially copyable for in-place storage reuse");

// Node/Cursor erase tree-sitter types behind opaque storage to keep
// <tree_sitter/api.h> out of the public header. Each constructor uses
// placement new to start the underlying tree-sitter object's lifetime inside
// the buffer, so subsequent reinterpret_cast accesses (wrapped in
// std::launder) refer to an object whose lifetime has formally begun, instead
// of relying on C++20's implicit-object creation rules.
static TSNode &nodeRef(unsigned char *S) {
  return *std::launder(reinterpret_cast<TSNode *>(S));
}
static const TSNode &nodeRef(const unsigned char *S) {
  return *std::launder(reinterpret_cast<const TSNode *>(S));
}

Node::Node() { ::new (static_cast<void *>(Storage)) TSNode{}; }
Node::Node(const Node &Other) noexcept {
  // Start a TSNode lifetime in the destination first, then byte-copy the
  // source representation — well-defined for trivially-copyable TSNode.
  ::new (static_cast<void *>(Storage)) TSNode{};
  std::memcpy(Storage, Other.Storage, sizeof(Storage));
}
Node::Node(Node &&Other) noexcept : Node(static_cast<const Node &>(Other)) {}
Node &Node::operator=(const Node &Other) noexcept {
  if (this != &Other) {
    std::memcpy(Storage, Other.Storage, sizeof(Storage));
  }
  return *this;
}
Node &Node::operator=(Node &&Other) noexcept {
  return *this = static_cast<const Node &>(Other);
}

std::string_view Node::type() const {
  const char *T = ts_node_type(nodeRef(Storage));
  return T ? std::string_view(T) : std::string_view();
}

std::string_view Node::text(std::string_view Source) const {
  uint32_t Start = startByte();
  uint32_t End = endByte();
  if (Start >= Source.size() || End > Source.size() || Start >= End) {
    return {};
  }
  return std::string_view(Source.data() + Start, End - Start);
}

uint32_t Node::startByte() const {
  return ts_node_start_byte(nodeRef(Storage));
}
uint32_t Node::endByte() const { return ts_node_end_byte(nodeRef(Storage)); }
uint32_t Node::startRow() const {
  return ts_node_start_point(nodeRef(Storage)).row;
}

uint32_t Node::childCount() const {
  return ts_node_child_count(nodeRef(Storage));
}
uint32_t Node::namedChildCount() const {
  return ts_node_named_child_count(nodeRef(Storage));
}

Node Node::child(uint32_t Index) const {
  Node Result;
  nodeRef(Result.Storage) = ts_node_child(nodeRef(Storage), Index);
  return Result;
}
Node Node::namedChild(uint32_t Index) const {
  Node Result;
  nodeRef(Result.Storage) = ts_node_named_child(nodeRef(Storage), Index);
  return Result;
}
Node Node::parent() const {
  Node Result;
  nodeRef(Result.Storage) = ts_node_parent(nodeRef(Storage));
  return Result;
}

bool Node::isNull() const { return ts_node_is_null(nodeRef(Storage)); }
bool Node::isNamed() const { return ts_node_is_named(nodeRef(Storage)); }
bool Node::hasError() const { return ts_node_has_error(nodeRef(Storage)); }
// --- Cursor ---

static TSTreeCursor &cursorRef(unsigned char *S) {
  return *std::launder(reinterpret_cast<TSTreeCursor *>(S));
}
static const TSTreeCursor &cursorRef(const unsigned char *S) {
  return *std::launder(reinterpret_cast<const TSTreeCursor *>(S));
}

static_assert(sizeof(TSTreeCursor) <= sizeof(Cursor::Storage),
              "Cursor::Storage too small for TSTreeCursor");
static_assert(std::is_trivially_copyable_v<TSTreeCursor>,
              "TSTreeCursor must be trivially copyable for bytewise move");

static bool shouldSkip(TSNode N) {
  using std::literals::operator""sv;
  if (!ts_node_is_named(N)) {
    return true;
  }
  // Skip annotation extras — they are transparent during conversion.
  // ts_node_type may return nullptr for unknown symbols; guard before
  // constructing a string_view to avoid UB.
  const char *T = ts_node_type(N);
  if (T != nullptr && std::string_view(T) == "annotation"sv) {
    return true;
  }
  return false;
}

void Cursor::skip() {
  while (Valid &&
         shouldSkip(ts_tree_cursor_current_node(&cursorRef(Storage)))) {
    Valid = ts_tree_cursor_goto_next_sibling(&cursorRef(Storage));
  }
}

Cursor::Cursor() : Storage{}, Valid(false), Live(false) {
  // Start a TSTreeCursor lifetime in the storage buffer so subsequent
  // launder-based accesses are well-defined under C++17. The placeholder is
  // overwritten via ts_tree_cursor_new in the Node-taking constructor.
  ::new (static_cast<void *>(Storage)) TSTreeCursor{};
}

Cursor::Cursor(const Node &Parent) : Valid(false), Live(false) {
  ::new (static_cast<void *>(Storage))
      TSTreeCursor(ts_tree_cursor_new(nodeRef(Parent.Storage)));
  Live = true;
  Valid = ts_tree_cursor_goto_first_child(&cursorRef(Storage));
  if (Valid) {
    skip();
  }
}

Cursor::~Cursor() {
  if (Live) {
    ts_tree_cursor_delete(&cursorRef(Storage));
  }
}

Cursor::Cursor(Cursor &&Other) noexcept : Valid(Other.Valid), Live(Other.Live) {
  // Start the destination's TSTreeCursor lifetime, then bytewise-copy the
  // source representation (well-defined for trivially-copyable TSTreeCursor).
  ::new (static_cast<void *>(Storage)) TSTreeCursor{};
  std::memcpy(Storage, Other.Storage, sizeof(Storage));
  // Reset the source so its destructor doesn't double-free the tree-sitter
  // internals we just transferred.
  std::memset(Other.Storage, 0, sizeof(Other.Storage));
  Other.Valid = false;
  Other.Live = false;
}

Cursor &Cursor::operator=(Cursor &&Other) noexcept {
  if (this != &Other) {
    if (Live) {
      ts_tree_cursor_delete(&cursorRef(Storage));
    }
    std::memcpy(Storage, Other.Storage, sizeof(Storage));
    std::memset(Other.Storage, 0, sizeof(Other.Storage));
    Valid = std::exchange(Other.Valid, false);
    Live = std::exchange(Other.Live, false);
  }
  return *this;
}

Cursor Cursor::copy() const {
  Cursor Result;
  if (!Live) {
    // No live TSTreeCursor to copy (default-constructed or moved-from): Storage
    // is zeroed and passing it to ts_tree_cursor_copy would be undefined
    // behavior. Return an equivalently empty cursor.
    return Result;
  }
  cursorRef(Result.Storage) = ts_tree_cursor_copy(&cursorRef(Storage));
  Result.Valid = Valid;
  Result.Live = true;
  return Result;
}

Node Cursor::node() const {
  if (!Valid) {
    return Node{};
  }
  Node Result;
  nodeRef(Result.Storage) = ts_tree_cursor_current_node(&cursorRef(Storage));
  return Result;
}

bool Cursor::valid() const { return Valid; }

bool Cursor::next() {
  if (!Valid) {
    return false;
  }
  Valid = ts_tree_cursor_goto_next_sibling(&cursorRef(Storage));
  if (Valid) {
    skip();
  }
  return Valid;
}

// --- Tree ---
struct Tree::Impl {
  TSTree *Raw;
  ~Impl() {
    if (Raw)
      ts_tree_delete(Raw);
  }
};

Tree::Tree(TSTree *RawTree) : Pimpl(std::make_unique<Impl>()) {
  Pimpl->Raw = RawTree;
}
Tree::~Tree() = default;
Tree::Tree(Tree &&Other) noexcept = default;
Tree &Tree::operator=(Tree &&Other) noexcept = default;

Node Tree::rootNode() const {
  Node Result;
  // ts_parser_parse_string can return a null tree (e.g. allocation failure),
  // in which case Pimpl->Raw is null. Return a default null Node so callers
  // observe the failure via Node::isNull() instead of dereferencing it here.
  if (Pimpl->Raw == nullptr) {
    return Result;
  }
  nodeRef(Result.Storage) = ts_tree_root_node(Pimpl->Raw);
  return Result;
}

// --- Parser ---
struct Parser::Impl {
  TSParser *Raw;
  ~Impl() {
    if (Raw)
      ts_parser_delete(Raw);
  }
};

Parser::Parser(LanguageFn Lang) : Pimpl(std::make_unique<Impl>()) {
  Pimpl->Raw = ts_parser_new();
  // ts_parser_new can return null (OOM), and ts_parser_set_language fails on a
  // language ABI mismatch. In either case drop the parser so parse() reports a
  // clean failure (null Tree) instead of dereferencing a null TSParser.
  if (Pimpl->Raw != nullptr && !ts_parser_set_language(Pimpl->Raw, Lang())) {
    ts_parser_delete(Pimpl->Raw);
    Pimpl->Raw = nullptr;
  }
}
Parser::~Parser() = default;
Parser::Parser(Parser &&Other) noexcept = default;
Parser &Parser::operator=(Parser &&Other) noexcept = default;

Tree Parser::parse(std::string_view Source) const {
  if (Pimpl->Raw == nullptr) {
    return Tree(nullptr);
  }
  TSTree *RawTree = ts_parser_parse_string(
      Pimpl->Raw, nullptr, Source.data(), static_cast<uint32_t>(Source.size()));
  return Tree(RawTree);
}

} // namespace WasmEdge::WAT
