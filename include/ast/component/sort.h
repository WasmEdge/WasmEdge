// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/ast/component/sort.h - Sort class definitions ------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Sort node class and enumerations.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/types.h"

#include <variant>

namespace WasmEdge {
namespace AST {
namespace Component {

// core:sortidx ::= sort:<core:sort> idx:<u32> => (sort idx)
// core:sort    ::= 0x00                       => func
//                | 0x01                       => table
//                | 0x02                       => memory
//                | 0x03                       => global
//                | 0x10                       => type
//                | 0x11                       => module
//                | 0x12                       => instance
// sortidx      ::= sort:<sort> idx:<u32>      => (sort idx)
// sort         ::= 0x00 cs:<core:sort>        => core cs
//                | 0x01                       => func
//                | 0x02                       => value 🪙
//                | 0x03                       => type
//                | 0x04                       => component
//                | 0x05                       => instance

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

/// AST Component::SortIndex class template.
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
