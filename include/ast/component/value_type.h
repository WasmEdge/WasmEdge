// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//=== ValueType class definitions
//
// Part of the WasmEdge Project.
//
//===------------------------------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the ValueType node classes.
///
//===------------------------------------------------------------------------------------------===//
#pragma once

#include "common/types.h"

namespace WasmEdge {
namespace AST {

enum class PrimitiveValueType : Byte {
  String = 0x73,
  Char = 0x74,
  Float64 = 0x75,
  Float32 = 0x76,
  U64 = 0x77,
  S64 = 0x78,
  U32 = 0x79,
  S32 = 0x7a,
  U16 = 0x7b,
  S16 = 0x7c,
  U8 = 0x7d,
  S8 = 0x7e,
  Bool = 0x7f
};

using ValueType = std::variant<uint32_t, PrimitiveValueType>;

} // namespace AST
} // namespace WasmEdge
