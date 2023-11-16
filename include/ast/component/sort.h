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
  Func = 0x00,
  Table = 0x01,
  Memory = 0x02,
  Global = 0x03,
  Type = 0x10,
  Module = 0x11,
  Instance = 0x12,
};

enum class Sort {
  Core = 0x00,
  Func = 0x01,
  Value = 0x02,
  Type = 0x03,
  Component = 0x04,
  Instance = 0x05,
};

} // namespace AST
} // namespace WasmEdge
