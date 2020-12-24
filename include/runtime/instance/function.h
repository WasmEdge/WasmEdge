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

#include "ast/instruction.h"
#include "module.h"
#include "runtime/hostfunc.h"

#include <memory>
#include <string>
#include <vector>

namespace SSVM {
namespace Runtime {
namespace Instance {

class FunctionInstance {
public:
  using CompiledFunction = void;

  FunctionInstance() = delete;
  /// Constructor for native function.
  FunctionInstance(const uint32_t ModAddr, const FType &Type,
                   Span<const std::pair<uint32_t, ValType>> Locs,
                   AST::InstrView Expr)
      : IsHostFunction(false), FuncType(Type), ModuleAddr(ModAddr),
        Locals(Locs.begin(), Locs.end()), Instrs(Expr.begin(), Expr.end()) {}
  /// Constructor for host function. Module address will not be used.
  FunctionInstance(std::unique_ptr<HostFunctionBase> &&Func)
      : IsHostFunction(true), FuncType(Func->getFuncType()), ModuleAddr(0),
        HostFunc(std::move(Func)) {}
  virtual ~FunctionInstance() = default;

  /// Getter of checking is host function.
  bool isHostFunction() const { return IsHostFunction; }

  /// Getter of module address of this function instance.
  uint32_t getModuleAddr() const { return ModuleAddr; }

  /// Setter of module address of this function instance.
  void setModuleAddr(const uint32_t Addr) { ModuleAddr = Addr; }

  /// Getter of function type.
  const FType &getFuncType() const { return FuncType; }

  /// Getter of function body instrs.
  Span<const std::pair<uint32_t, ValType>> getLocals() const { return Locals; }

  /// Getter of function body instrs.
  AST::InstrView getInstrs() const { return Instrs; }

  /// Getter of symbol
  const auto getSymbol() const noexcept { return Symbol; }
  /// Setter of symbol
  void setSymbol(DLSymbol<CompiledFunction> S) noexcept {
    Symbol = std::move(S);
  }

  /// Getter of host function.
  HostFunctionBase &getHostFunc() const { return *HostFunc.get(); }

private:
  const bool IsHostFunction;
  const FType &FuncType;

  /// \name Data of function instance for native function.
  /// @{
  uint32_t ModuleAddr;
  const std::vector<std::pair<uint32_t, ValType>> Locals;
  const AST::InstrVec Instrs;
  DLSymbol<CompiledFunction> Symbol;
  /// @}

  /// \name Data of function instance for host function.
  /// @{
  std::unique_ptr<HostFunctionBase> HostFunc;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace SSVM
