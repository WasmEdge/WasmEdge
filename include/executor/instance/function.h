// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/executor/instance/function.h - Function Instance definition --===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the function instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/instruction.h"
#include "executor/common.h"
#include "executor/instance/entity.h"
#include "executor/instance/module.h"
#include <memory>
#include <string>
#include <vector>

namespace SSVM {
namespace Executor {
namespace Instance {

class FunctionInstance : public Entity {
public:
  FunctionInstance(bool IsHost = false) : IsHostFunction(IsHost){};
  virtual ~FunctionInstance() = default;

  /// Set the module instance index in store manager.
  ErrCode setModuleAddr(unsigned int Addr);

  /// Set the function type in module instance.
  ErrCode setFuncType(ModuleInstance::FType *Type);

  /// Set the host function class.
  ErrCode setHostFuncAddr(unsigned int Addr);

  /// Move the local variables in code section into function instance.
  ErrCode
  setLocals(const std::vector<std::pair<unsigned int, AST::ValType>> &Loc);

  /// Move the instruction list in code segment into function instance.
  ErrCode setInstrs(AST::InstrVec &Expr);

  /// Getter of function type.
  ModuleInstance::FType *getFuncType() const { return FuncType; }

  /// Getter of module address of this function instance.
  unsigned int getModuleAddr() const { return ModuleAddr; }

  /// Getter of function body instrs.
  const std::vector<std::pair<unsigned int, AST::ValType>> &getLocals() const {
    return Locals;
  }

  /// Getter of function body instrs.
  const AST::InstrVec &getInstrs() const { return Instrs; }

  /// Getter of host function.
  unsigned int getHostFuncAddr() const { return HostFuncAddr; }

  /// Getter of checking is host function.
  bool isHostFunction() const { return IsHostFunction; }

private:
  bool IsHostFunction;

  /// \name Data of function instance for native function.
  /// @{
  ModuleInstance::FType *FuncType;
  unsigned int ModuleAddr;
  std::vector<std::pair<unsigned int, AST::ValType>> Locals;
  AST::InstrVec Instrs;
  /// @}

  /// \name Data of function instance for host function.
  /// @{
  unsigned int HostFuncAddr;
  /// @}
};

} // namespace Instance
} // namespace Executor
} // namespace SSVM
