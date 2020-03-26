// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/runtime/instance/function.h - Function Instance definition ---===//
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

#include "common/ast/instruction.h"
#include "runtime/hostfunc.h"
#include "module.h"

#include <memory>
#include <string>
#include <vector>

namespace SSVM {
namespace Runtime {
namespace Instance {

class FunctionInstance {
public:
  FunctionInstance() = delete;
  /// Constructor for native function.
  FunctionInstance(const uint32_t ModAddr, const FType &Type,
                   const std::vector<std::pair<uint32_t, ValType>> &Locs,
                   const AST::InstrVec &Expr)
      : IsHostFunction(false), ModuleAddr(ModAddr), FuncType(Type),
        Locals(Locs) {
    /// Copy instructions
    for (auto &It : Expr) {
      if (auto Res = makeInstructionNode(*It.get())) {
        Instrs.push_back(std::move(*Res));
      }
    }
  }
  /// Constructor for host function. Module address will not be used.
  FunctionInstance(std::unique_ptr<HostFunctionBase> &Func)
      : IsHostFunction(true), ModuleAddr(0), FuncType(Func->getFuncType()),
        HostFunc(std::move(Func)) {}
  virtual ~FunctionInstance() = default;

  /// Getter of checking is host function.
  const bool isHostFunction() const { return IsHostFunction; }

  /// Getter of module address of this function instance.
  const uint32_t getModuleAddr() const { return ModuleAddr; }

  /// Getter of function type.
  const FType &getFuncType() const { return FuncType; }

  /// Getter of function body instrs.
  const std::vector<std::pair<uint32_t, ValType>> &getLocals() const {
    return Locals;
  }

  /// Getter of function body instrs.
  const AST::InstrVec &getInstrs() const { return Instrs; }

  /// Getter of host function.
  HostFunctionBase &getHostFunc() const { return *HostFunc.get(); }

private:
  const bool IsHostFunction;
  const FType &FuncType;

  /// \name Data of function instance for native function.
  /// @{
  const uint32_t ModuleAddr;
  const std::vector<std::pair<uint32_t, ValType>> Locals;
  AST::InstrVec Instrs;
  /// @}

  /// \name Data of function instance for host function.
  /// @{
  std::unique_ptr<HostFunctionBase> HostFunc;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace SSVM
