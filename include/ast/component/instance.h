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

class InstantiateArg {
public:
  std::string_view getName() const noexcept { return Name; }
  std::string &getName() noexcept { return Name; }
  uint32_t getInstanceIdx() const noexcept { return InstanceIndex; }
  uint32_t &getInstanceIdx() noexcept { return InstanceIndex; }

private:
  std::string Name;
  uint32_t InstanceIndex;
};

class InlineExport {
public:
  std::string_view getName() const noexcept { return Name; }
  std::string &getName() noexcept { return Name; }
  SortIdx getSortIdx() const noexcept { return SortIdx; }
  SortIdx &getSortIdx() noexcept { return SortIdx; }

private:
  std::string Name;
  SortIdx SortIdx;
};

class Instantiate {
public:
  Instantiate() noexcept : ModuleIdx{0}, Args{} {}
  Instantiate(uint32_t Idx, std::vector<InstantiateArg> Args) noexcept
      : ModuleIdx{Idx}, Args{Args} {}

  uint32_t getModuleIdx() const noexcept { return ModuleIdx; }
  Span<const InstantiateArg> getArgs() const noexcept { return Args; }

private:
  uint32_t ModuleIdx;
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

using CoreInstanceExpr = std::variant<Instantiate, InlineExports>;

} // namespace AST
} // namespace WasmEdge
