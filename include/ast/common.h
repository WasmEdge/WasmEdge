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

namespace AST {

/// Value types enumeration class.
enum class ValType : unsigned char {
  None = 0x40,
  I32 = 0x7F,
  I64 = 0x7E,
  F32 = 0x7D,
  F64 = 0x7C
};

/// Element types enumeration class.
enum class ElemType : unsigned char { Func = 0x60, FuncRef = 0x70 };

/// Value mutability enumeration class.
enum class ValMut : unsigned char { Const = 0x00, Var = 0x01 };

} // namespace AST