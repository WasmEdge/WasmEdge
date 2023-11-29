// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

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

class CoreInstantiateArg {
public:
  std::string_view getName() const noexcept { return Name; }
  std::string &getName() noexcept { return Name; }
  uint32_t getInstanceIdx() const noexcept { return InstanceIndex; }
  uint32_t &getInstanceIdx() noexcept { return InstanceIndex; }

private:
  std::string Name;
  uint32_t InstanceIndex;
};

class CoreInlineExport {
public:
  std::string_view getName() const noexcept { return Name; }
  std::string &getName() noexcept { return Name; }
  SortIndex getSortIdx() const noexcept { return SortIdx; }
  SortIndex &getSortIdx() noexcept { return SortIdx; }

private:
  std::string Name;
  SortIndex SortIdx;
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

class CoreInlineExports {
public:
  CoreInlineExports() noexcept : Exports{} {}
  CoreInlineExports(std::vector<CoreInlineExport> Es) noexcept : Exports{Es} {}

  Span<const CoreInlineExport> getExports() const noexcept { return Exports; }

private:
  std::vector<CoreInlineExport> Exports;
};

class InstantiateArg {
public:
  std::string_view getName() const noexcept { return Name; }
  std::string &getName() noexcept { return Name; }
  SortIndex getSortIndex() const noexcept { return Idx; }
  SortIndex &getSortIndex() noexcept { return Idx; }

private:
  std::string Name;
  SortIndex Idx;
};

class InlineExport {
public:
  std::string_view getName() const noexcept { return Name; }
  std::string &getName() noexcept { return Name; }
  SortIndex getSortIdx() const noexcept { return SortIdx; }
  SortIndex &getSortIdx() noexcept { return SortIdx; }

private:
  std::string Name;
  SortIndex SortIdx;
};

class Instantiate {
public:
  Instantiate() noexcept : ComponentIndex{0}, Args{} {}
  Instantiate(uint32_t Idx, std::vector<InstantiateArg> Args) noexcept
      : ComponentIndex{Idx}, Args{Args} {}

  uint32_t getComponentIdx() const noexcept { return ComponentIndex; }
  Span<const InstantiateArg> getArgs() const noexcept { return Args; }

private:
  uint32_t ComponentIndex;
  std::vector<InstantiateArg> Args;
};

class InlineExports {
public:
  InlineExports() noexcept : Exports{} {}
  InlineExports(std::vector<InlineExport> Es) noexcept : Exports{Es} {}

  Span<const InlineExport> getExports() const noexcept { return Exports; }

private:
  std::vector<InlineExport> Exports;
};

using CoreInstanceExpr = std::variant<CoreInstantiate, CoreInlineExports>;
using InstanceExpr = std::variant<Instantiate, InlineExports>;

} // namespace AST
} // namespace WasmEdge
