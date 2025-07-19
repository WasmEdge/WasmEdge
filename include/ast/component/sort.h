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

namespace WasmEdge {
namespace AST {
namespace Component {

/// NOTE: The `sort` may be the `core:sort` case. Therefore it's more convenient
/// to implement the `sort` and `core:sort` into a struct.

// core:sort ::= 0x00                       => func
//             | 0x01                       => table
//             | 0x02                       => memory
//             | 0x03                       => global
//             | 0x10                       => type
//             | 0x11                       => module
//             | 0x12                       => instance
// sort      ::= 0x00 cs:<core:sort>        => core cs
//             | 0x01                       => func
//             | 0x02                       => value 🪙
//             | 0x03                       => type
//             | 0x04                       => component
//             | 0x05                       => instance

/// AST Component::Sort node.
class Sort {
public:
  enum class CoreSortType : uint8_t {
    Func = 0x00,
    Table = 0x01,
    Memory = 0x02,
    Global = 0x03,
    Type = 0x10,
    Module = 0x11,
    Instance = 0x12,
    Max
  };

  enum class SortType : uint8_t {
    Func = 0x01,
    Value = 0x02,
    Type = 0x03,
    Component = 0x04,
    Instance = 0x05,
    Max
  };

  bool isCore() const noexcept { return IsCore; }
  void setIsCore(const bool C) noexcept { IsCore = C; }

  CoreSortType getCoreSortType() const noexcept { return Type.CS; }
  void setCoreSortType(const CoreSortType T) noexcept { Type.CS = T; }

  SortType getSortType() const noexcept { return Type.S; }
  void setSortType(const SortType T) noexcept { Type.S = T; }

private:
  union {
    CoreSortType CS;
    SortType S;
  } Type;
  bool IsCore;
};

// core:sortidx ::= sort:<core:sort> idx:<u32> => (sort idx)
// sortidx      ::= sort:<sort> idx:<u32>      => (sort idx)

/// AST Component::SortIndex node.
class SortIndex {
public:
  const Sort &getSort() const noexcept { return S; }
  Sort &getSort() noexcept { return S; }

  uint32_t getIdx() const noexcept { return Idx; }
  void setIdx(const uint32_t I) noexcept { Idx = I; }

private:
  Sort S;
  uint32_t Idx;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
