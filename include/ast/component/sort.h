// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

//===-- wasmedge/ast/component/sort.h - sort class definitions ------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Sort node class
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/expression.h"
#include "ast/type.h"

#include <vector>

namespace WasmEdge {
namespace AST {

enum class Sort {
  CoreFunc,
  CoreTable,
  CoreMemory,
  CoreGlobal,
  CoreType,
  CoreInstance,
  Func,
  Value,
  Type,
  Component,
  Instance,
};

// core:sortidx        ::= sort:<core:sort> idx:<u32>
class SortIndex {
public:
  Sort getSort() const noexcept { return S; }
  Sort &getSort() noexcept { return S; }
  uint32_t getSortIdx() const noexcept { return Idx; }
  uint32_t &getSortIdx() noexcept { return Idx; }

private:
  Sort S;
  uint32_t Idx;
};

} // namespace AST
} // namespace WasmEdge
