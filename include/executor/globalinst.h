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
#include <vector>

namespace SSVM {

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

  /// Get the value of this instance.
  template <typename T> Executor::ErrCode getValue(T &Val);

  /// Set the value of this instance.
  template <typename T> Executor::ErrCode setValue(T Val);

  /// Global Instance address in store manager.
  unsigned int Addr;

private:
  /// \name Data of global instance.
  /// @{
  AST::ValType Type;
  AST::ValMut Mut;
  AST::ValVariant Value;
  std::vector<std::unique_ptr<AST::Instruction>> Instrs;
  /// @}
};

} // namespace SSVM