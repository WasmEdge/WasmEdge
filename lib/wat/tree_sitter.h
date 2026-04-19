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
  explicit Cursor(Node Parent);
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
