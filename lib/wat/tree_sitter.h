// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

struct TSLanguage;
struct TSNode;
struct TSTree;
struct TSParser;
struct TSTreeCursor;

namespace WasmEdge::WAT {

class Tree;

class Node {
public:
  Node();
  // Explicit copy/move so the destination has a TSNode lifetime started in
  // its Storage buffer before the bytes are assigned. The implicit forms
  // would only byte-copy Storage without placement-new, leaving subsequent
  // std::launder-based access to nodeRef() formally undefined under C++17.
  Node(const Node &Other) noexcept;
  Node(Node &&Other) noexcept;
  Node &operator=(const Node &Other) noexcept;
  Node &operator=(Node &&Other) noexcept;
  ~Node() = default;

  std::string_view type() const;
  std::string_view text(std::string_view Source) const;

  uint32_t startByte() const;
  uint32_t endByte() const;
  uint32_t startRow() const;

  uint32_t childCount() const;
  uint32_t namedChildCount() const;
  Node child(uint32_t Index) const;
  Node namedChild(uint32_t Index) const;
  Node parent() const;

  bool isNull() const;
  bool isNamed() const;
  bool hasError() const;

  // Storage is accessible within this translation unit (private header).
  // Not exposed to public API consumers.
  friend class Tree;
  alignas(8) unsigned char Storage[32];
};

class Tree {
public:
  ~Tree();
  Tree(Tree &&Other) noexcept;
  Tree &operator=(Tree &&Other) noexcept;
  Tree(const Tree &) = delete;
  Tree &operator=(const Tree &) = delete;

  Node rootNode() const;

private:
  friend class Parser;
  explicit Tree(TSTree *RawTree);
  struct Impl;
  std::unique_ptr<Impl> Pimpl;
};

class Cursor {
public:
  /// Create a cursor over the named children of Parent.
  /// Positioned at the first named child, or exhausted if none exist.
  explicit Cursor(const Node &Parent);
  ~Cursor();

  Cursor(Cursor &&Other) noexcept;
  Cursor &operator=(Cursor &&Other) noexcept;
  Cursor(const Cursor &) = delete;
  Cursor &operator=(const Cursor &) = delete;

  /// Create an independent deep copy at the same position.
  Cursor copy() const;

  /// Get the current named child node. Null if exhausted.
  Node node() const;

  /// Whether the cursor is positioned on a valid named child.
  bool valid() const;

  /// Advance to the next named sibling. Returns valid().
  bool next();

  alignas(8) unsigned char Storage[32];

private:
  Cursor();
  bool Valid;
  // Whether Storage holds a live TSTreeCursor that must be deleted. False for a
  // default-constructed or moved-from cursor (Storage zeroed); tree-sitter does
  // not guarantee that deleting a zeroed cursor is safe. Distinct from Valid,
  // which only tracks whether the cursor sits on a named child.
  bool Live;

  void skip();
};

class Parser {
public:
  using LanguageFn = const TSLanguage *(*)();
  explicit Parser(LanguageFn Lang);
  ~Parser();
  Parser(Parser &&Other) noexcept;
  Parser &operator=(Parser &&Other) noexcept;
  Parser(const Parser &) = delete;
  Parser &operator=(const Parser &) = delete;

  Tree parse(std::string_view Source) const;

private:
  struct Impl;
  std::unique_ptr<Impl> Pimpl;
};

} // namespace WasmEdge::WAT
