//===-- ssvm/ast/common.h - Types definition --------------------*- C++ -*-===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the enumerations of AST nodes.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "variant.h"
#include <cstdint>

namespace SSVM {
namespace AST {

/// Value types enumeration class.
enum class ValType : unsigned char {
  None = 0x40,
  I32 = 0x7F,
  I64 = 0x7E,
  F32 = 0x7D,
  F64 = 0x7C
};

using ValVariant = Variant<uint32_t, uint64_t, float, double>;

inline constexpr ValVariant ValueFromType(ValType Type) noexcept {
  switch (Type) {
  default:
  case ValType::I32:
    return uint32_t(0U);
  case ValType::I64:
    return uint64_t(0U);
  case ValType::F32:
    return float(0.0F);
  case ValType::F64:
    return double(0.0);
  }
}

/// Element types enumeration class.
enum class ElemType : unsigned char { Func = 0x60, FuncRef = 0x70 };

/// Value mutability enumeration class.
enum class ValMut : unsigned char { Const = 0x00, Var = 0x01 };

} // namespace AST
} // namespace SSVM
