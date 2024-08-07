// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
namespace Component {

enum class CoreSort : Byte {
  Func = 0x00,
  Table = 0x01,
  Memory = 0x02,
  Global = 0x03,
  Type = 0x10,
  Module = 0x11,
  Instance = 0x12,
};
enum class SortCase : Byte {
  Func = 0x01,
  Value = 0x02,
  Type = 0x03,
  Component = 0x04,
  Instance = 0x05,
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

} // namespace Component
} // namespace AST
} // namespace WasmEdge
