//===-- ssvm/executor/globalinst.h - Global Instance definition -----------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the global instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/common.h"
#include "ast/instruction.h"
#include "common.h"
#include <memory>

class GlobalInstance {
public:
  GlobalInstance() = default;
  ~GlobalInstance() = default;

  /// Set the global type.
  Executor::ErrCode setGlobalType(AST::ValType &ValueType,
                                  AST::ValMut &Mutibility);

  /// Move the instruction list in global segment into global instance.
  Executor::ErrCode
  setExpression(std::vector<std::unique_ptr<AST::Instruction>> &Expr);

  /// Set the value of this instance.
  Executor::ErrCode setValue(std::variant<int32_t, int64_t, float, double> Val);

  /// Global Instance ID in store manager.
  unsigned int Id;

private:
  /// \name Data of global instance.
  /// @{
  AST::ValType Type;
  AST::ValMut Mut;
  std::variant<int32_t, int64_t, float, double> Val;
  std::vector<std::unique_ptr<AST::Instruction>> Instrs;
  /// @}
};