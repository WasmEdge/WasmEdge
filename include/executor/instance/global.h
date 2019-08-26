//===-- ssvm/executor/instance/global.h - Global Instance definition ------===//
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
#include "executor/common.h"
#include <memory>
#include <vector>

namespace SSVM {
namespace Executor {
namespace Instance {

class GlobalInstance {
public:
  GlobalInstance() = default;
  ~GlobalInstance() = default;

  /// Set the global type.
  ErrCode setGlobalType(AST::ValType &ValueType, AST::ValMut &Mutibility);

  /// Move the instruction list in global segment into global instance.
  ErrCode setInstrs(std::vector<std::unique_ptr<AST::Instruction>> &Expr);

  /// Set the value of this instance.
  template <typename T> ErrCode setValue(T &Val);

  /// Get the value of this instance.
  template <typename T> ErrCode getValue(T &Val);

  /// Getter of function body instrs.
  const auto &getInstrs() const { return Instrs; }

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

} // namespace Instance
} // namespace Executor
} // namespace SSVM
