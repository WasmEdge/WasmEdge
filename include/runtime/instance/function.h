// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

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

#include <memory>
#include <string>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ModuleInstance;

class FunctionInstance {
public:
  using CompiledFunction = void;

  FunctionInstance() = delete;
  /// Move constructor.
  FunctionInstance(FunctionInstance &&Inst) noexcept
      : ModInst(Inst.ModInst), Addr(Inst.Addr), FuncType(Inst.FuncType),
        Data(std::move(Inst.Data)) {}
  /// Constructor for native function.
  FunctionInstance(ModuleInstance *Mod, const AST::FunctionType &Type,
                   Span<const std::pair<uint32_t, ValType>> Locs,
                   AST::InstrView Expr) noexcept
      : ModInst(Mod), Addr(0), FuncType(Type),
        Data(std::in_place_type_t<WasmFunction>(), Locs, Expr) {}
  /// Constructor for compiled function.
  FunctionInstance(ModuleInstance *Mod, const AST::FunctionType &Type,
                   Symbol<CompiledFunction> S) noexcept
      : ModInst(Mod), Addr(0), FuncType(Type),
        Data(std::in_place_type_t<Symbol<CompiledFunction>>(), std::move(S)) {}
  /// Constructor for host function. Module instance will not be used.
  FunctionInstance(std::unique_ptr<HostFunctionBase> &&Func) noexcept
      : ModInst(nullptr), Addr(0), FuncType(Func->getFuncType()),
        Data(std::in_place_type_t<std::unique_ptr<HostFunctionBase>>(),
             std::move(Func)) {}

  virtual ~FunctionInstance() = default;

  /// Getter of checking is native wasm function.
  bool isWasmFunction() const noexcept {
    return std::holds_alternative<WasmFunction>(Data);
  }

  /// Getter of checking is compiled function.
  bool isCompiledFunction() const noexcept {
    return std::holds_alternative<Symbol<CompiledFunction>>(Data);
  }

  /// Getter of checking is host function.
  bool isHostFunction() const noexcept {
    return std::holds_alternative<std::unique_ptr<HostFunctionBase>>(Data);
  }

  /// Getter of module instance of this function instance.
  ModuleInstance *getModule() const noexcept { return ModInst; }

  /// Getter and setter of function address in store manager.
  uint32_t getAddr() const noexcept { return Addr; }
  void setAddr(uint32_t A) noexcept { Addr = A; }

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
  auto &getSymbol() const noexcept {
    return *std::get_if<Symbol<CompiledFunction>>(&Data);
  }

  /// Getter of host function.
  HostFunctionBase &getHostFunc() const noexcept {
    return *std::get_if<std::unique_ptr<HostFunctionBase>>(&Data)->get();
  }

private:
  struct WasmFunction {
    const std::vector<std::pair<uint32_t, ValType>> Locals;
    AST::InstrVec Instrs;
    WasmFunction(Span<const std::pair<uint32_t, ValType>> Locs,
                 AST::InstrView Expr) noexcept
        : Locals(Locs.begin(), Locs.end()) {
      // FIXME: Modify the capacity to prevent from connection of 2 vectors.
      Instrs.reserve(Expr.size() + 1);
      Instrs.assign(Expr.begin(), Expr.end());
    }
  };

  /// \name Data of function instance.
  /// @{
  ModuleInstance *ModInst;
  uint32_t Addr;
  const AST::FunctionType &FuncType;
  std::variant<WasmFunction, Symbol<CompiledFunction>,
               std::unique_ptr<HostFunctionBase>>
      Data;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
