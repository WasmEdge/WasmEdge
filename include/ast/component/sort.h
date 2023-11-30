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

enum class CoreSort {
  Func,
  Table,
  Memory,
  Global,
  Type,
  Module,
  Instance,
};
enum class SortCase {
  CoreInstance,
  Func,
  Value,
  Type,
  Component,
  Instance,
};
using Sort = std::variant<CoreSort, SortCase>;

// core:sortidx        ::= sort:<core:sort> idx:<u32>
template <typename SortType> class SortIndex {
public:
  SortType getSort() const noexcept { return St; }
  SortType &getSort() noexcept { return St; }
  uint32_t getSortIdx() const noexcept { return Idx; }
  uint32_t &getSortIdx() noexcept { return Idx; }

private:
  SortType St;
  uint32_t Idx;
};

} // namespace AST
} // namespace WasmEdge
