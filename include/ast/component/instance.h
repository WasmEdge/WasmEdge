// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/ast/component/instance.h - Instance class definitions ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Instance node related classes.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/sort.h"
#include "common/span.h"

#include <string>
#include <variant>
#include <vector>

namespace WasmEdge {
namespace AST {
namespace Component {

// core:instantiatearg ::= n:<core:name> 0x12 i:<instanceidx>
//                       => (with n (instance i))
// instantiatearg      ::= n:<name>  si:<sortidx>
//                       => (with n si)

/// AST Component::InstantiateArg class template.
template <typename IndexType> class InstantiateArg {
public:
  std::string_view getName() const noexcept { return Name; }
  std::string &getName() noexcept { return Name; }
  const IndexType &getIndex() const noexcept { return Idx; }
  IndexType &getIndex() noexcept { return Idx; }

private:
  std::string Name;
  IndexType Idx;
};

// core:inlineexport   ::= n:<core:name> si:<core:sortidx>
//                       => (export n si)
// inlineexport        ::= n:<exportname> si:<sortidx>
//                       => (export n si)

/// AST Component::InlineExport class.
class InlineExport {
public:
  std::string_view getName() const noexcept { return Name; }
  std::string &getName() noexcept { return Name; }
  const SortIndex &getSortIdx() const noexcept { return SortIdx; }
  SortIndex &getSortIdx() noexcept { return SortIdx; }

private:
  std::string Name;
  SortIndex SortIdx;
};

// core:instance       ::= ie:<core:instanceexpr>
//                       => (instance ie)
// core:instanceexpr   ::= 0x00 m:<moduleidx> arg*:vec(<core:instantiatearg>)
//                       => (instantiate m arg*)
//                       | 0x01 e*:vec(<core:inlineexport>)
//                       => e*

/// AST Component::CoreInstance node.
class CoreInstance {
public:
  using InstantiateArgs = std::vector<InstantiateArg<uint32_t>>;
  using InlineExports = std::vector<InlineExport>;

  void setInstantiateArgs(const uint32_t ModIdx,
                          InstantiateArgs &&Args) noexcept {
    Expr.emplace<std::pair<uint32_t, InstantiateArgs>>(ModIdx, std::move(Args));
  }
  uint32_t getModuleIndex() const noexcept {
    return std::get_if<std::pair<uint32_t, InstantiateArgs>>(&Expr)->first;
  }
  Span<const InstantiateArg<uint32_t>> getInstantiateArgs() const noexcept {
    return std::get_if<std::pair<uint32_t, InstantiateArgs>>(&Expr)->second;
  }

  void setInlineExports(InlineExports &&Exports) noexcept {
    Expr.emplace<InlineExports>(std::move(Exports));
  }
  Span<const InlineExport> getInlineExports() const noexcept {
    return *std::get_if<InlineExports>(&Expr);
  }

  bool isInstantiateModule() const noexcept {
    return std::holds_alternative<std::pair<uint32_t, InstantiateArgs>>(Expr);
  }

  bool isInlineExport() const noexcept {
    return std::holds_alternative<InlineExports>(Expr);
  }

private:
  std::variant<std::pair<uint32_t, InstantiateArgs>, InlineExports> Expr;
};

// instance            ::= ie:<instanceexpr>
//                       => (instance ie)
// instanceexpr        ::= 0x00 c:<componentidx> arg*:vec(<instantiatearg>)
//                       => (instantiate c arg*)
//                       | 0x01 e*:vec(<inlineexport>)
//                       => e*

/// AST Component::Instance node.
class Instance {
public:
  using InstantiateArgs = std::vector<InstantiateArg<SortIndex>>;
  using InlineExports = std::vector<InlineExport>;

  void setInstantiateArgs(const uint32_t CompIdx,
                          InstantiateArgs &&Args) noexcept {
    Expr.emplace<std::pair<uint32_t, InstantiateArgs>>(CompIdx,
                                                       std::move(Args));
  }
  uint32_t getComponentIndex() const noexcept {
    return std::get_if<std::pair<uint32_t, InstantiateArgs>>(&Expr)->first;
  }
  Span<const InstantiateArg<SortIndex>> getInstantiateArgs() const noexcept {
    return std::get_if<std::pair<uint32_t, InstantiateArgs>>(&Expr)->second;
  }

  void setInlineExports(InlineExports &&Exports) noexcept {
    Expr.emplace<InlineExports>(std::move(Exports));
  }
  Span<const InlineExport> getInlineExports() const noexcept {
    return *std::get_if<InlineExports>(&Expr);
  }

  bool isInstantiateModule() const noexcept {
    return std::holds_alternative<std::pair<uint32_t, InstantiateArgs>>(Expr);
  }

  bool isInlineExport() const noexcept {
    return std::holds_alternative<InlineExports>(Expr);
  }

private:
  std::variant<std::pair<uint32_t, InstantiateArgs>, InlineExports> Expr;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
