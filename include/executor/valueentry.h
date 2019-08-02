//===-- ssvm/executor/valueentry.h - Value Entry class definition ---------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of Value Entry class in stack manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/common.h"
#include "common.h"
#include <variant>

class ValueEntry {
public:
  /// Constructors for assigning values. For instructions using.
  ValueEntry(int32_t Val) : Type(AST::ValType::I32), Value(Val){};
  ValueEntry(int64_t Val) : Type(AST::ValType::I64), Value(Val){};
  ValueEntry(float Val) : Type(AST::ValType::F32), Value(Val){};
  ValueEntry(double Val) : Type(AST::ValType::F64), Value(Val){};

  ~ValueEntry() = default;

  /// Getters of getting values.
  Executor::ErrCode getValueI32(int32_t &Val);
  Executor::ErrCode getValueI64(int64_t &Val);
  Executor::ErrCode getValueF32(float &Val);
  Executor::ErrCode getValueF64(double &Val);

private:
  /// \name Data of value entry.
  /// @{
  AST::ValType Type;
  std::variant<int32_t, int64_t, float, double> Value;
  /// @}
};