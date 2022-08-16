// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//=== wasmedge/ast/component/sort.h - sort type definitions
//
// Part of the WasmEdge Project.
//
//===------------------------------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the sort node classes.
///
//===------------------------------------------------------------------------------------------===//
#pragma once

#include "ast/description.h"
#include "common/types.h"

namespace WasmEdge {
namespace AST {

using CoreSort = Byte;

enum class Sort {
  CoreFunc,
  Table,
  Memory,
  Global,
  CoreType,
  Module,
  CoreInstance,
  Func,
  Value,
  Type,
  Component,
  Instance
};

class SortIndex {
public:
  Sort &getSort() noexcept { return Sort; }
  const Sort &getSort() const noexcept { return Sort; }

  void setIndex(uint32_t V) noexcept { Index = V; }
  uint32_t getIndex() const noexcept { return Index; }

private:
  Sort Sort;
  uint32_t Index;
};

} // namespace AST
} // namespace WasmEdge
