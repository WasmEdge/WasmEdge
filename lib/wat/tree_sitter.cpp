// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "tree_sitter.h"

#include <tree_sitter/api.h>

#include <cstring>
#include <utility>

extern "C" const TSLanguage *tree_sitter_wat();

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
uint32_t Node::startColumn() const {
  return ts_node_start_point(nodeRef(Storage)).column;
}
uint32_t Node::endRow() const {
  return ts_node_end_point(nodeRef(Storage)).row;
}
uint32_t Node::endColumn() const {
  return ts_node_end_point(nodeRef(Storage)).column;
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
Node Node::childByFieldName(std::string_view Name) const {
  Node Result;
  nodeRef(Result.Storage) =
      ts_node_child_by_field_name(nodeRef(Storage), Name.data(), Name.size());
  return Result;
}
Node Node::nextSibling() const {
  Node Result;
  nodeRef(Result.Storage) = ts_node_next_sibling(nodeRef(Storage));
  return Result;
}
Node Node::prevSibling() const {
  Node Result;
  nodeRef(Result.Storage) = ts_node_prev_sibling(nodeRef(Storage));
  return Result;
}
Node Node::nextNamedSibling() const {
  Node Result;
  nodeRef(Result.Storage) = ts_node_next_named_sibling(nodeRef(Storage));
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
bool Node::isMissing() const { return ts_node_is_missing(nodeRef(Storage)); }

// --- Cursor ---
struct Cursor::Impl {
  TSTreeCursor Raw;
  ~Impl() { ts_tree_cursor_delete(&Raw); }
};

Cursor::Cursor(const Node &RootNode) : Pimpl(std::make_unique<Impl>()) {
  Pimpl->Raw = ts_tree_cursor_new(nodeRef(RootNode.Storage));
}
Cursor::~Cursor() = default;
Cursor::Cursor(Cursor &&Other) noexcept = default;
Cursor &Cursor::operator=(Cursor &&Other) noexcept = default;

bool Cursor::gotoFirstChild() {
  return ts_tree_cursor_goto_first_child(&Pimpl->Raw);
}
bool Cursor::gotoNextSibling() {
  return ts_tree_cursor_goto_next_sibling(&Pimpl->Raw);
}
bool Cursor::gotoParent() { return ts_tree_cursor_goto_parent(&Pimpl->Raw); }
Node Cursor::currentNode() const {
  Node Result;
  nodeRef(Result.Storage) = ts_tree_cursor_current_node(&Pimpl->Raw);
  return Result;
}
std::string_view Cursor::currentFieldName() const {
  const char *N = ts_tree_cursor_current_field_name(&Pimpl->Raw);
  return N ? std::string_view(N) : std::string_view();
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

Parser::Parser() : Pimpl(std::make_unique<Impl>()) {
  Pimpl->Raw = ts_parser_new();
  ts_parser_set_language(Pimpl->Raw, tree_sitter_wat());
}
Parser::~Parser() = default;
Parser::Parser(Parser &&Other) noexcept = default;
Parser &Parser::operator=(Parser &&Other) noexcept = default;

Tree Parser::parse(std::string_view Source) const {
  TSTree *RawTree =
      ts_parser_parse_string(Pimpl->Raw, nullptr, Source.data(), Source.size());
  return Tree(RawTree);
}

} // namespace WasmEdge::WAT
