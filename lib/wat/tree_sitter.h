// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

struct TSNode;
struct TSTree;
struct TSParser;
struct TSTreeCursor;

namespace WasmEdge::WAT {

class Tree;
class Cursor;

class Node {
public:
  Node();

  std::string_view type() const;
  std::string_view text(std::string_view Source) const;

  uint32_t startByte() const;
  uint32_t endByte() const;
  uint32_t startRow() const;
  uint32_t startColumn() const;
  uint32_t endRow() const;
  uint32_t endColumn() const;

  uint32_t childCount() const;
  uint32_t namedChildCount() const;
  Node child(uint32_t Index) const;
  Node namedChild(uint32_t Index) const;
  Node childByFieldName(std::string_view Name) const;
  Node nextSibling() const;
  Node prevSibling() const;
  Node nextNamedSibling() const;
  Node parent() const;

  bool isNull() const;
  bool isNamed() const;
  bool hasError() const;
  bool isMissing() const;

  // Storage is accessible within this translation unit (private header).
  // Not exposed to public API consumers.
  friend class Tree;
  friend class Cursor;
  alignas(8) unsigned char Storage[32];
};

class Cursor {
public:
  explicit Cursor(const Node &RootNode);
  ~Cursor();
  Cursor(Cursor &&Other) noexcept;
  Cursor &operator=(Cursor &&Other) noexcept;
  Cursor(const Cursor &) = delete;
  Cursor &operator=(const Cursor &) = delete;

  bool gotoFirstChild();
  bool gotoNextSibling();
  bool gotoParent();
  Node currentNode() const;
  std::string_view currentFieldName() const;

private:
  struct Impl;
  std::unique_ptr<Impl> Pimpl;
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

class Parser {
public:
  Parser();
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
