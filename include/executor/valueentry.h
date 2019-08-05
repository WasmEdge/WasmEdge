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
#include <cstdint>
#include <variant>

class ValueEntry {
public:
  /// Constructors for assigning values. For instructions using.
  explicit ValueEntry(int32_t Val) : Type(AST::ValType::I32), Value(Val){};
  explicit ValueEntry(int64_t Val) : Type(AST::ValType::I64), Value(Val){};
  explicit ValueEntry(float Val) : Type(AST::ValType::F32), Value(Val){};
  explicit ValueEntry(double Val) : Type(AST::ValType::F64), Value(Val){};

  ~ValueEntry() = default;

  /// Getter of value type.
  Executor::ErrCode getType(AST::ValType &T);

  /// Getters of getting values.
  template <typename T> Executor::ErrCode getValue(T &Val);

private:
  /// \name Data of value entry.
  /// @{
  AST::ValType Type;
  AST::ValVariant Value;
  /// @}
};