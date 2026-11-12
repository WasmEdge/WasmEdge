// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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
#include "runtime/instance/composite.h"
#include "runtime/refheap.h"
#include "runtime/typetable.h"

#include <cstddef>
#include <memory>
#include <numeric>
#include <optional>
#include <string>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {
class FunctionInstance;
} // namespace Instance

template <> struct HeapTraits<Instance::FunctionInstance> {
  static constexpr HeapKind Kind = HeapKind::Function;
};

namespace Instance {

class ModuleInstance;

class FunctionInstance : public CompositeBase {
public:
  using CompiledFunction = void;

  FunctionInstance() = delete;
  FunctionInstance(const FunctionInstance &) = delete;
  FunctionInstance(FunctionInstance &&) = delete;
  FunctionInstance &operator=(const FunctionInstance &) = delete;
  FunctionInstance &operator=(FunctionInstance &&) = delete;

  /// Create a function instance in a slot of the global reference heap.
  template <typename... ArgsT>
  static Expect<UniqueHeapPtr<FunctionInstance>>
  create(ArgsT &&...Args) noexcept {
    auto *Heap = RefHeap::getGlobal();
    if (Heap == nullptr) {
      return Unexpect(ErrCode::Value::RefHeapFull);
    }
    return Heap->makeUnique<FunctionInstance>(std::forward<ArgsT>(Args)...);
  }

private:
  friend class Runtime::RefHeap;

  /// Constructor for native function.
  FunctionInstance(const ModuleInstance *Mod, const uint32_t TIdx,
                   const AST::FunctionType &Type,
                   Span<const std::pair<uint32_t, ValType>> Locs,
                   AST::InstrView Expr) noexcept
      : CompositeBase(Mod, TIdx), FuncType(Type),
        Data(std::in_place_type_t<WasmFunction>(), Locs, Expr) {
    assuming(ModInst);
  }
  /// Constructor for compiled function.
  FunctionInstance(const ModuleInstance *Mod, const uint32_t TIdx,
                   const AST::FunctionType &Type,
                   Symbol<CompiledFunction> S) noexcept
      : CompositeBase(Mod, TIdx), CompiledCode(S.get()), FuncType(Type),
        Data(std::in_place_type_t<Symbol<CompiledFunction>>(), std::move(S)) {
    assuming(ModInst);
  }
  /// Constructors for host function.
  FunctionInstance(const ModuleInstance *Mod, const uint32_t TIdx,
                   std::unique_ptr<HostFunctionBase> &&Func) noexcept
      : CompositeBase(Mod, TIdx), FuncType(Func->getFuncType()),
        Data(std::in_place_type_t<std::unique_ptr<HostFunctionBase>>(),
             std::move(Func)) {
    assuming(ModInst);
  }
  FunctionInstance(std::unique_ptr<HostFunctionBase> &&Func) noexcept
      : CompositeBase(), FuncType(Func->getFuncType()),
        Data(std::in_place_type_t<std::unique_ptr<HostFunctionBase>>(),
             std::move(Func)) {
    const AST::SubType *Group[] = {&getHostFunc().getDefinedType()};
    if (auto Id =
            CanonicalTypeTable::getGlobal().registerRecGroup(Group, 0, {})) {
      HostTypeId = *Id;
    }
  }

public:
  /// Check whether this is a native wasm function.
  bool isWasmFunction() const noexcept {
    return std::holds_alternative<WasmFunction>(Data);
  }

  /// Check whether this is a compiled function.
  bool isCompiledFunction() const noexcept {
    return std::holds_alternative<Symbol<CompiledFunction>>(Data);
  }

  /// Check whether this is a host function.
  bool isHostFunction() const noexcept {
    return std::holds_alternative<std::unique_ptr<HostFunctionBase>>(Data);
  }

  /// Getter for function type.
  const AST::FunctionType &getFuncType() const noexcept { return FuncType; }

  /// Getter for function local variables.
  Span<const std::pair<uint32_t, ValType>> getLocals() const noexcept {
    return std::get_if<WasmFunction>(&Data)->Locals;
  }

  /// Getter for function local number.
  uint32_t getLocalNum() const noexcept {
    return std::get_if<WasmFunction>(&Data)->LocalNum;
  }

  /// Getter for function body instrs.
  AST::InstrView getInstrs() const noexcept {
    if (std::holds_alternative<WasmFunction>(Data)) {
      return std::get<WasmFunction>(Data).Instrs;
    } else {
      return {};
    }
  }

  /// Getter for symbol.
  auto &getSymbol() const noexcept {
    return *std::get_if<Symbol<CompiledFunction>>(&Data);
  }

  /// Getter for host function.
  HostFunctionBase &getHostFunc() const noexcept {
    return *std::get_if<std::unique_ptr<HostFunctionBase>>(&Data)->get();
  }

  /// Getter for the canonical type id of a host function out of any module.
  std::optional<uint32_t> getHostCanonicalTypeId() const noexcept {
    return HostTypeId;
  }

  /// Field offsets read inline by the AOT/JIT compiler for the call_indirect
  /// fast path; offsetof-derived so they follow the target ABI.
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
  static constexpr uint64_t getModuleOffset() noexcept {
    return offsetof(FunctionInstance, ModInst);
  }
  static constexpr uint64_t getTypeIndexOffset() noexcept {
    return offsetof(FunctionInstance, TypeIdx);
  }
  static constexpr uint64_t getCompiledCodeOffset() noexcept {
    return offsetof(FunctionInstance, CompiledCode);
  }
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

  /// Upgrade from WasmFunction to CompiledFunction.
  /// Must be called under synchronization that
  /// prevents concurrent access to this function's body.
  bool unsafeUpgradeToCompiled(Symbol<CompiledFunction> Sym) noexcept {
    if (!isWasmFunction()) {
      return false;
    }
    CompiledCode = Sym.get();
    Data = std::move(Sym);
    return true;
  }

private:
  struct WasmFunction {
    const std::vector<std::pair<uint32_t, ValType>> Locals;
    const uint32_t LocalNum;
    AST::InstrVec Instrs;
    WasmFunction(Span<const std::pair<uint32_t, ValType>> Locs,
                 AST::InstrView Expr) noexcept
        : Locals(Locs.begin(), Locs.end()),
          LocalNum(
              std::accumulate(Locals.begin(), Locals.end(), UINT32_C(0),
                              [](uint32_t N, const auto &Pair) -> uint32_t {
                                return N + Pair.first;
                              })) {
      // FIXME: Modify the capacity to prevent connecting 2 vectors.
      Instrs.reserve(Expr.size() + 1);
      Instrs.assign(Expr.begin(), Expr.end());
    }
  };

  /// \name Data of function instance.
  /// @{

  /// Compiled code pointer, read inline at getCompiledCodeOffset() by the
  /// call_indirect fast path.
  void *CompiledCode = nullptr;
  const AST::FunctionType &FuncType;
  std::variant<WasmFunction, Symbol<CompiledFunction>,
               std::unique_ptr<HostFunctionBase>>
      Data;
  std::optional<uint32_t> HostTypeId;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
