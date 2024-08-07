// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/ast/component/instance.h - instance class definitions ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Instance node class
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/sort.h"
#include "ast/expression.h"
#include "ast/type.h"

#include <vector>

namespace WasmEdge {
namespace AST {
namespace Component {

template <typename IndexType> class InstantiateArg {
public:
  std::string_view getName() const noexcept { return Name; }
  std::string &getName() noexcept { return Name; }
  IndexType getIndex() const noexcept { return Idx; }
  IndexType &getIndex() noexcept { return Idx; }

private:
  std::string Name;
  IndexType Idx;
};

using CoreInstantiateArg = InstantiateArg<uint32_t>;

template <typename SortType> class InlineExport {
public:
  std::string_view getName() const noexcept { return Name; }
  std::string &getName() noexcept { return Name; }
  SortIndex<SortType> getSortIdx() const noexcept { return SortIdx; }
  SortIndex<SortType> &getSortIdx() noexcept { return SortIdx; }

private:
  std::string Name;
  SortIndex<SortType> SortIdx;
};

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

class Instantiate {
public:
  Instantiate() noexcept : ComponentIndex{0}, Args{} {}
  Instantiate(uint32_t Idx,
              std::vector<InstantiateArg<SortIndex<Sort>>> Args) noexcept
      : ComponentIndex{Idx}, Args{Args} {}

  uint32_t getComponentIdx() const noexcept { return ComponentIndex; }
  Span<const InstantiateArg<SortIndex<Sort>>> getArgs() const noexcept {
    return Args;
  }

private:
  uint32_t ComponentIndex;
  std::vector<InstantiateArg<SortIndex<Sort>>> Args;
};

template <typename SortType> class InlineExports {
public:
  InlineExports() noexcept : Exports{} {}
  InlineExports(std::vector<InlineExport<SortType>> Es) noexcept
      : Exports{Es} {}

  Span<const InlineExport<SortType>> getExports() const noexcept {
    return Exports;
  }

private:
  std::vector<InlineExport<SortType>> Exports;
};

using CoreInlineExports = InlineExports<CoreSort>;
using CompInlineExports = InlineExports<Sort>;

using CoreInstanceExpr = std::variant<CoreInstantiate, CoreInlineExports>;
using InstanceExpr = std::variant<Instantiate, CompInlineExports>;

} // namespace Component
} // namespace AST
} // namespace WasmEdge
