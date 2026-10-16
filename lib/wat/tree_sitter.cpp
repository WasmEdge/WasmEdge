// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "tree_sitter.h"

#include "wat/wat_util.h"

#include <tree_sitter/api.h>

#include <limits>
#include <utility>
#include <vector>

using namespace std::string_view_literals;

namespace WasmEdge {
namespace WAT {

// Every accessor is null-safe. The tree-sitter functions dereference the node
// id. Cursor::node() gives a null node at the end of the children,
// firstErrorChild() gives one if no child has an error, and parent() gives one
// at the root. A null node reads as an empty leaf with no text.

TSSymbol Node::symbol() const { return isNull() ? 0 : ts_node_symbol(Raw); }

std::string_view Node::text(std::string_view Source) const {
  if (isNull()) {
    return {};
  }
  uint32_t Start = startByte();
  uint32_t End = endByte();
  if (Start >= Source.size() || End > Source.size() || Start >= End) {
    return {};
  }
  return std::string_view(Source.data() + Start, End - Start);
}

uint32_t Node::startByte() const {
  return isNull() ? 0 : ts_node_start_byte(Raw);
}
uint32_t Node::endByte() const { return isNull() ? 0 : ts_node_end_byte(Raw); }
uint32_t Node::startRow() const {
  return isNull() ? 0 : ts_node_start_point(Raw).row;
}

Node Node::parent() const {
  Node Result;
  if (!isNull()) {
    Result.Raw = ts_node_parent(Raw);
  }
  return Result;
}

bool Node::isNull() const { return ts_node_is_null(Raw); }
bool Node::hasError() const { return !isNull() && ts_node_has_error(Raw); }

Node Node::firstErrorChild() const {
  Node Result;
  if (isNull()) {
    return Result;
  }
  // Use a tree-sitter cursor for the same reason as forEachLeaf.
  TSTreeCursor Cursor = ts_tree_cursor_new(Raw);
  for (bool Valid = ts_tree_cursor_goto_first_child(&Cursor); Valid;
       Valid = ts_tree_cursor_goto_next_sibling(&Cursor)) {
    TSNode Child = ts_tree_cursor_current_node(&Cursor);
    if (ts_node_is_error(Child) || ts_node_has_error(Child)) {
      Result.Raw = Child;
      break;
    }
  }
  ts_tree_cursor_delete(&Cursor);
  return Result;
}

// --- Cursor ---

// Skip the anonymous tokens and the annotations.
void Cursor::skip() {
  while (Valid) {
    const TSNode N = ts_tree_cursor_current_node(&Raw);
    if (ts_node_is_named(N) && !Node::isAnnotation(N)) {
      return;
    }
    Valid = ts_tree_cursor_goto_next_sibling(&Raw);
  }
}

Cursor::Cursor() : Raw{}, Valid(false) {}

Cursor::Cursor(Node Parent) : Raw{}, Valid(false) {
  // A cursor on a null node stays at the end. ts_tree_cursor_new dereferences
  // the node, so it must not see a null node. The zero storage is safe for
  // ts_tree_cursor_delete, as in the move constructor.
  if (Parent.isNull()) {
    return;
  }
  Raw = ts_tree_cursor_new(Parent.Raw);
  Valid = ts_tree_cursor_goto_first_child(&Raw);
  if (Valid) {
    skip();
  }
}

Cursor::~Cursor() { ts_tree_cursor_delete(&Raw); }

void Cursor::steal(Cursor &Other) noexcept {
  // Fill the source with zeros, so that its destructor does not free the
  // stack.
  Raw = std::exchange(Other.Raw, TSTreeCursor{});
  Valid = std::exchange(Other.Valid, false);
}

Cursor::Cursor(Cursor &&Other) noexcept { steal(Other); }

Cursor &Cursor::operator=(Cursor &&Other) noexcept {
  if (this != &Other) {
    ts_tree_cursor_delete(&Raw);
    steal(Other);
  }
  return *this;
}

Cursor Cursor::copy() const {
  Cursor Result;
  Result.Raw = ts_tree_cursor_copy(&Raw);
  Result.Valid = Valid;
  return Result;
}

Node Cursor::node() const {
  if (!Valid) {
    return Node{};
  }
  Node Result;
  Result.Raw = ts_tree_cursor_current_node(&Raw);
  return Result;
}

bool Cursor::valid() const { return Valid; }

bool Cursor::next() {
  if (!Valid) {
    return false;
  }
  Valid = ts_tree_cursor_goto_next_sibling(&Raw);
  if (Valid) {
    skip();
  }
  return Valid;
}

// --- Tree ---
void Tree::Deleter::operator()(TSTree *T) const noexcept { ts_tree_delete(T); }

Node Tree::rootNode() const {
  // A call to ts_tree_root_node(nullptr) is undefined behavior. For a null
  // tree, this function returns a null Node.
  if (!Raw) {
    return Node{};
  }
  Node Result;
  Result.Raw = ts_tree_root_node(Raw.get());
  return Result;
}

// --- Parser ---
void Parser::Deleter::operator()(TSParser *P) const noexcept {
  ts_parser_delete(P);
}

Parser::Parser(LanguageFn Lang) : Raw(ts_parser_new()) {
  ts_parser_set_language(Raw.get(), Lang());
}

Tree Parser::parse(std::string_view Source) const {
  // Refuse input that is too large. The length parameter of
  // ts_parser_parse_string is a uint32_t, and it truncates such input to a
  // shorter prefix.
  if (Source.size() > std::numeric_limits<uint32_t>::max()) {
    return Tree(nullptr);
  }
  TSTree *RawTree = ts_parser_parse_string(
      Raw.get(), nullptr, Source.data(), static_cast<uint32_t>(Source.size()));
  return Tree(RawTree);
}

namespace {
// Map each grammar symbol to its NodeType once. A lookup by symbol is an
// array index, and a lookup by name is a string compare for each entry.
struct GrammarSymbols {
  std::vector<NodeType> Types;
  TSSymbol Annotation = 0;
  GrammarSymbols() {
    static constexpr std::pair<std::string_view, NodeType> Names[] = {
        {"keyword"sv, NodeType::Keyword},
        {"sexpr"sv, NodeType::Sexpr},
        {"u"sv, NodeType::U},
        {"id"sv, NodeType::Id},
        {"s"sv, NodeType::S},
        {"f"sv, NodeType::F},
        {"string"sv, NodeType::String},
        {"root"sv, NodeType::Sexpr},
        {"reserved"sv, NodeType::Reserved},
    };
    const TSLanguage *Lang = tree_sitter_wat();
    const uint32_t Count = ts_language_symbol_count(Lang);
    Types.resize(Count, NodeType::Unknown);
    for (uint32_t I = 0; I < Count; ++I) {
      const TSSymbol Sym = static_cast<TSSymbol>(I);
      const char *Name = ts_language_symbol_name(Lang, Sym);
      if (!Name) {
        continue;
      }
      Types[I] = lookupKeyword(Names, Name).value_or(NodeType::Unknown);
      if (Name == "annotation"sv) {
        Annotation = Sym;
      }
    }
  }
};
const GrammarSymbols &grammarSymbols() {
  static const GrammarSymbols Symbols;
  return Symbols;
}
} // namespace

NodeType nodeType(Node N) {
  if (N.isNull()) {
    return NodeType::Unknown;
  }
  // The ERROR node of tree-sitter has the built-in symbol 0xFFFF. It is not in
  // the symbol table of the grammar.
  const TSSymbol Sym = N.symbol();
  if (Sym == std::numeric_limits<TSSymbol>::max()) {
    return NodeType::Error;
  }
  const auto &Types = grammarSymbols().Types;
  return Sym < Types.size() ? Types[Sym] : NodeType::Unknown;
}

// Return true if N is an annotation token. An annotation is an extra of the
// grammar, and Cursor skips it.
bool Node::isAnnotation(TSNode N) {
  return ts_node_symbol(N) == grammarSymbols().Annotation;
}

} // namespace WAT
} // namespace WasmEdge
