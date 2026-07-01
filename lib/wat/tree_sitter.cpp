// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "tree_sitter.h"

#include <tree_sitter/api.h>

#include <cstring>
#include <utility>

namespace WasmEdge::WAT {

static_assert(sizeof(TSNode) <= sizeof(Node::Storage),
              "Node::Storage too small for TSNode");

static TSNode &nodeRef(unsigned char *S) {
  return *reinterpret_cast<TSNode *>(S);
}
static const TSNode &nodeRef(const unsigned char *S) {
  return *reinterpret_cast<const TSNode *>(S);
}

Node::Node() { std::memset(Storage, 0, sizeof(Storage)); }

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
  return *reinterpret_cast<TSTreeCursor *>(S);
}
static const TSTreeCursor &cursorRef(const unsigned char *S) {
  return *reinterpret_cast<const TSTreeCursor *>(S);
}

static_assert(sizeof(TSTreeCursor) <= sizeof(Cursor::Storage),
              "Cursor::Storage too small for TSTreeCursor");

static bool shouldSkip(TSNode N) {
  using std::literals::operator""sv;
  if (!ts_node_is_named(N)) {
    return true;
  }
  // Skip annotation extras — they are transparent during conversion.
  const char *T = ts_node_type(N);
  if (T == "annotation"sv) {
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

Cursor::Cursor() : Storage{}, Valid(false) {}

Cursor::Cursor(Node Parent) : Valid(false) {
  cursorRef(Storage) = ts_tree_cursor_new(nodeRef(Parent.Storage));
  Valid = ts_tree_cursor_goto_first_child(&cursorRef(Storage));
  if (Valid) {
    skip();
  }
}

Cursor::~Cursor() { ts_tree_cursor_delete(&cursorRef(Storage)); }

Cursor::Cursor(Cursor &&Other) noexcept : Valid(Other.Valid) {
  std::copy(Other.Storage, Other.Storage + sizeof(Storage), Storage);
  // Zero out the source so its destructor doesn't free the stack.
  std::fill(Other.Storage, Other.Storage + sizeof(Other.Storage),
            static_cast<uint8_t>(0));
  Other.Valid = false;
}

Cursor &Cursor::operator=(Cursor &&Other) noexcept {
  if (this != &Other) {
    ts_tree_cursor_delete(&cursorRef(Storage));
    std::copy(Other.Storage, Other.Storage + sizeof(Storage), Storage);
    std::fill(Other.Storage, Other.Storage + sizeof(Other.Storage),
              static_cast<uint8_t>(0));
    Valid = std::exchange(Other.Valid, false);
  }
  return *this;
}

Cursor Cursor::copy() const {
  Cursor Result;
  cursorRef(Result.Storage) = ts_tree_cursor_copy(&cursorRef(Storage));
  Result.Valid = Valid;
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
  ts_parser_set_language(Pimpl->Raw, Lang());
}
Parser::~Parser() = default;
Parser::Parser(Parser &&Other) noexcept = default;
Parser &Parser::operator=(Parser &&Other) noexcept = default;

Tree Parser::parse(std::string_view Source) const {
  TSTree *RawTree = ts_parser_parse_string(
      Pimpl->Raw, nullptr, Source.data(), static_cast<uint32_t>(Source.size()));
  return Tree(RawTree);
}

} // namespace WasmEdge::WAT
