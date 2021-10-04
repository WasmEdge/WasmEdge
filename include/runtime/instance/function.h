// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/runtime/instance/function.h - Function Instance definition ==//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the function instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/instruction.h"
#include "common/symbol.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/module.h"

#include <memory>
#include <string>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class FunctionInstance {
public:
  using CompiledFunction = void;

  FunctionInstance() = delete;
  /// Move constructor.
  FunctionInstance(FunctionInstance &&Inst) noexcept
      : ModuleAddr(Inst.ModuleAddr), FuncType(Inst.FuncType),
        Data(std::move(Inst.Data)) {}
  /// Constructor for native function.
  FunctionInstance(const uint32_t ModAddr, const AST::FunctionType &Type,
                   Span<const std::pair<uint32_t, ValType>> Locs,
                   AST::InstrView Expr) noexcept
      : ModuleAddr(ModAddr), FuncType(Type),
        Data(std::in_place_type_t<WasmFunction>(), Locs, Expr) {}
  /// Constructor for compiled function.
  FunctionInstance(const uint32_t ModAddr, const AST::FunctionType &Type,
                   Symbol<CompiledFunction> S) noexcept
      : ModuleAddr(ModAddr), FuncType(Type),
        Data(std::in_place_type_t<Symbol<CompiledFunction>>(), std::move(S)) {}
  /// Constructor for host function. Module address will not be used.
  FunctionInstance(std::unique_ptr<HostFunctionBase> &&Func) noexcept
      : ModuleAddr(0), FuncType(Func->getFuncType()),
        Data(std::in_place_type_t<std::unique_ptr<HostFunctionBase>>(),
             std::move(Func)) {}

  virtual ~FunctionInstance() = default;

  /// Getter of checking is native wasm function.
  bool isWasmFunction() const {
    return std::holds_alternative<WasmFunction>(Data);
  }

  /// Getter of checking is compiled function.
  bool isCompiledFunction() const {
    return std::holds_alternative<Symbol<CompiledFunction>>(Data);
  }

  /// Getter of checking is host function.
  bool isHostFunction() const {
    return std::holds_alternative<std::unique_ptr<HostFunctionBase>>(Data);
  }

  /// Getter of module address of this function instance.
  uint32_t getModuleAddr() const { return ModuleAddr; }

  /// Getter of function type.
  const AST::FunctionType &getFuncType() const { return FuncType; }

  /// Getter of function local variables.
  Span<const std::pair<uint32_t, ValType>> getLocals() const noexcept {
    return std::get_if<WasmFunction>(&Data)->Locals;
  }

  /// Getter of function body instrs.
  AST::InstrView getInstrs() const noexcept {
    if (std::holds_alternative<WasmFunction>(Data)) {
      return std::get<WasmFunction>(Data).Instrs;
    } else {
      return {};
    }
  }

  /// Getter of symbol
  auto getSymbol() const noexcept {
    return *std::get_if<Symbol<CompiledFunction>>(&Data);
  }

  /// Getter of host function.
  HostFunctionBase &getHostFunc() const {
    return *std::get_if<std::unique_ptr<HostFunctionBase>>(&Data)->get();
  }

private:
  struct WasmFunction {
    const std::vector<std::pair<uint32_t, ValType>> Locals;
    const AST::InstrVec Instrs;
    WasmFunction(Span<const std::pair<uint32_t, ValType>> Locs,
                 AST::InstrView Expr) noexcept
        : Locals(Locs.begin(), Locs.end()), Instrs(Expr.begin(), Expr.end()) {}
  };

  /// \name Data of function instance.
  /// @{
  const uint32_t ModuleAddr;
  const AST::FunctionType &FuncType;
  std::variant<WasmFunction, Symbol<CompiledFunction>,
               std::unique_ptr<HostFunctionBase>>
      Data;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
