//===-- ssvm/executor/entry/value.h - Value Entry class definition --------===//
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
#include "executor/common.h"
#include <cstdint>
#include <variant>

namespace SSVM {
namespace Executor {

class ValueEntry {
public:
  /// Default constructors for temp ValueEntry.
  ValueEntry() : Type(AST::ValType::I32), Value(0) {}
  /// Copy constructor for duplication.
  explicit ValueEntry(const ValueEntry &VE) : Type(VE.Type), Value(VE.Value) {}
  explicit ValueEntry(AST::ValType &VT, AST::ValVariant &Val)
      : Type(VT), Value(Val) {}
  /// Constructors for the different value type
  explicit ValueEntry(int32_t Val) : Type(AST::ValType::I32), Value(Val) {}
  explicit ValueEntry(int64_t Val) : Type(AST::ValType::I64), Value(Val) {}
  explicit ValueEntry(float Val) : Type(AST::ValType::F32), Value(Val) {}
  explicit ValueEntry(double Val) : Type(AST::ValType::F64), Value(Val) {}

  ~ValueEntry() = default;

  /// Getter of value type.
  AST::ValType getType() const { return Type; }

  /// Value setters
  template <typename T> ErrCode setValue(T &Val);

  /// Getters of getting values.
  template <typename T> ErrCode getValue(T &Val) const;

private:
  /// \name Data of value entry.
  /// @{
  AST::ValType Type;
  AST::ValVariant Value;
  /// @}
};

} // namespace Executor
} // namespace SSVM
