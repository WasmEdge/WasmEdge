// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include <tree_sitter/api.h>

#include <cstdint>
#include <memory>
#include <string_view>

extern "C" const TSLanguage *tree_sitter_wat();

namespace WasmEdge {
namespace WAT {

class Tree;

class Node {
public:
  Node() noexcept = default;

  TSSymbol symbol() const;
  std::string_view text(std::string_view Source) const;

  uint32_t startByte() const;
  uint32_t endByte() const;
  uint32_t startRow() const;

  Node parent() const;

  bool isNull() const;
  bool hasError() const;

  /// Give the first child that is an error node or that contains an error. The
  /// result is null if no child has an error.
  Node firstErrorChild() const;

  /// Call Visit(Leaf, IsAnnotation) for each leaf node of this subtree in
  /// source order. An annotation is an extra of the grammar, and Cursor skips
  /// it. As a result, this walk is the only way to see it.
  template <typename F> void forEachLeaf(F &&Visit) const;

private:
  friend class Tree;
  friend class Cursor;
  static bool isAnnotation(TSNode N);
  TSNode Raw{};
};

template <typename F> void Node::forEachLeaf(F &&Visit) const {
  if (isNull()) {
    return;
  }
  // Use a tree-sitter cursor, because it moves to the next sibling in O(1)
  // time. An indexed child access needs O(i) time for each call, so a simple
  // walk needs O(n^2) time on a wide tree. A deep error-recovery node makes
  // such a wide tree.
  TSTreeCursor C = ts_tree_cursor_new(Raw);
  for (;;) {
    // Descend to the leftmost leaf.
    while (ts_tree_cursor_goto_first_child(&C)) {
    }
    Node Leaf;
    Leaf.Raw = ts_tree_cursor_current_node(&C);
    Visit(Leaf, isAnnotation(Leaf.Raw));
    // Move to the next sibling. If there is no sibling, ascend first. The walk
    // is complete when the cursor is back at the root.
    while (!ts_tree_cursor_goto_next_sibling(&C)) {
      if (!ts_tree_cursor_goto_parent(&C)) {
        ts_tree_cursor_delete(&C);
        return;
      }
    }
  }
}

class Tree {
public:
  Node rootNode() const;

private:
  friend class Parser;
  explicit Tree(TSTree *RawTree) noexcept : Raw(RawTree) {}
  struct Deleter {
    void operator()(TSTree *T) const noexcept;
  };
  std::unique_ptr<TSTree, Deleter> Raw;
};

class Cursor {
public:
  /// Make a cursor for the named children of Parent. The cursor starts at the
  /// first child. If Parent has no named child, the cursor is not valid.
  explicit Cursor(Node Parent);
  ~Cursor();

  Cursor(Cursor &&Other) noexcept;
  Cursor &operator=(Cursor &&Other) noexcept;
  Cursor(const Cursor &) = delete;
  Cursor &operator=(const Cursor &) = delete;

  /// Make an independent deep copy at the same position.
  Cursor copy() const;

  /// Get the current named child node. The result is null if the cursor is at
  /// the end.
  Node node() const;

  /// Return true if the cursor is at a valid named child.
  bool valid() const;

  /// Move the cursor to the next named sibling. The function returns valid().
  bool next();

private:
  Cursor();
  TSTreeCursor Raw;
  bool Valid;

  void skip();
  /// Take the state of Other, and leave Other empty and not valid.
  void steal(Cursor &Other) noexcept;
};

class Parser {
public:
  using LanguageFn = const TSLanguage *(*)();
  explicit Parser(LanguageFn Lang);

  Tree parse(std::string_view Source) const;

private:
  struct Deleter {
    void operator()(TSParser *P) const noexcept;
  };
  std::unique_ptr<TSParser, Deleter> Raw;
};

/// The node types of the simplified grammar. The grammar gives only the
/// S-expression structure and the raw token types. The converter gets the
/// semantics when it matches the keywords.
enum class NodeType {
  Sexpr, // The tree structure. Root and sexpr both map here.

  // Leaf token types
  Keyword,
  U,
  S,
  F,
  String,
  Id,
  Reserved,

  Error,   // A built-in type of tree-sitter.
  Unknown, // The fallback for a type that the converter does not know.
};

/// Give the node type of N. A null node is Unknown.
NodeType nodeType(Node N);

} // namespace WAT
} // namespace WasmEdge
