// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/common/types.h - Types definition ------------------*- C++ -*-===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the enumerations of Wasm VM used types.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <cstdint>

namespace SSVM {

/// Value types enumeration class.
enum class ValType : uint8_t {
  None = 0x40,
  I32 = 0x7F,
  I64 = 0x7E,
  F32 = 0x7D,
  F64 = 0x7C
};

/// Element types enumeration class.
enum class ElemType : uint8_t { Func = 0x60, FuncRef = 0x70 };

/// Value mutability enumeration class.
enum class ValMut : uint8_t { Const = 0x00, Var = 0x01 };

} // namespace SSVM
