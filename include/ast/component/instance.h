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
template <typename IndexType> class InstantiateArgImpl {
public:
  std::string_view getName() const noexcept { return Name; }
  std::string &getName() noexcept { return Name; }
  IndexType getIndex() const noexcept { return Idx; }
  IndexType &getIndex() noexcept { return Idx; }

private:
  std::string Name;
  IndexType Idx;
};

/// AST Component::CoreInstantiateArg aliasing.
using CoreInstantiateArg = InstantiateArgImpl<uint32_t>;
/// AST Component::InstantiateArg aliasing.
using InstantiateArg = InstantiateArgImpl<SortIndex<Sort>>;

// core:inlineexport   ::= n:<core:name> si:<core:sortidx>
//                       => (export n si)
// inlineexport        ::= n:<exportname> si:<sortidx>
//                       => (export n si)

/// AST Component::InlineExport class template.
template <typename SortType> class InlineExportImpl {
public:
  std::string_view getName() const noexcept { return Name; }
  std::string &getName() noexcept { return Name; }
  SortIndex<SortType> getSortIdx() const noexcept { return SortIdx; }
  SortIndex<SortType> &getSortIdx() noexcept { return SortIdx; }

private:
  std::string Name;
  SortIndex<SortType> SortIdx;
};

/// AST Component::InlineExportVec class template. For InlineExportImpl vector.
template <typename SortType> class InlineExportImplVec {
public:
  InlineExportImplVec() noexcept = default;
  InlineExportImplVec(std::vector<InlineExportImpl<SortType>> &&Es) noexcept
      : Exports(std::move(Es)) {}
  InlineExportImplVec(
      const std::vector<InlineExportImpl<SortType>> &Es) noexcept
      : Exports(Es) {}

  Span<const InlineExportImpl<SortType>> getExports() const noexcept {
    return Exports;
  }

private:
  std::vector<InlineExportImpl<SortType>> Exports;
};

// TODO: COMPONENT - The InlineExportImplVec is only the vector of InlineExport.
// This class can be removed in the future.

/// AST Component::CoreInlineExports aliasing.
using CoreInlineExports = InlineExportImplVec<CoreSort>;
/// AST Component::InlineExports aliasing.
using InlineExports = InlineExportImplVec<Sort>;

// core:instance       ::= ie:<core:instanceexpr>
//                       => (instance ie)
// core:instanceexpr   ::= 0x00 m:<moduleidx> arg*:vec(<core:instantiatearg>)
//                       => (instantiate m arg*)
//                       | 0x01 e*:vec(<core:inlineexport>)
//                       => e*
// instance            ::= ie:<instanceexpr>
//                       => (instance ie)
// instanceexpr        ::= 0x00 c:<componentidx> arg*:vec(<instantiatearg>)
//                       => (instantiate c arg*)
//                       | 0x01 e*:vec(<inlineexport>)
//                       => e*

/// AST Component::CoreInstantiate class.
class CoreInstantiate {
public:
  CoreInstantiate() noexcept : ModuleIdx{0}, Args{} {}
  CoreInstantiate(uint32_t Idx, std::vector<CoreInstantiateArg> Args) noexcept
      : ModuleIdx{Idx}, Args{Args} {}

  uint32_t getModuleIdx() const noexcept { return ModuleIdx; }
  Span<const CoreInstantiateArg> getArgs() const noexcept { return Args; }

private:
  uint32_t ModuleIdx;
  std::vector<CoreInstantiateArg> Args;
};

/// AST Component::Instantiate class.
class Instantiate {
public:
  Instantiate() noexcept : ComponentIndex{0}, Args{} {}
  Instantiate(uint32_t Idx, std::vector<InstantiateArg> &&Args) noexcept
      : ComponentIndex{Idx}, Args{std::move(Args)} {}

  uint32_t getComponentIdx() const noexcept { return ComponentIndex; }
  Span<const InstantiateArg> getArgs() const noexcept { return Args; }

private:
  uint32_t ComponentIndex;
  std::vector<InstantiateArg> Args;
};

// TODO: COMPONENT - Re-construct the Expr variant into a class and rename in
// the future.

/// AST Component::CoreInstanceExpr aliasing. (For AST Component::CoreInstance)
using CoreInstanceExpr = std::variant<CoreInstantiate, CoreInlineExports>;
/// AST Component::InstanceExpr aliasing. (For AST Component::Instance)
using InstanceExpr = std::variant<Instantiate, InlineExports>;

} // namespace Component
} // namespace AST
} // namespace WasmEdge
