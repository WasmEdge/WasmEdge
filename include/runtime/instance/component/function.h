// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors
#pragma once

#include "ast/component/type.h"
#include "common/types.h"
#include "runtime/instance/function.h"
#include "runtime/instance/memory.h"

#include <functional>
#include <memory>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ComponentInstance; // forward decl for parent component pointer

namespace Component {

class FunctionInstance {
  // A component function instance is either instantiated by `canon lift`
  // (guest) or supplied by the embedder as a host function consuming and
  // producing component-level values directly.
public:
  /// Host callback: component-level values in, (value, type) pairs out.
  using HostFuncCallback = std::function<
      Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>(
          Span<const ComponentValVariant>)>;

  FunctionInstance() = delete;
  /// Move constructor.
  FunctionInstance(FunctionInstance &&Inst) noexcept
      : OwnedFuncType(std::move(Inst.OwnedFuncType)),
        FuncType(OwnedFuncType ? *OwnedFuncType : Inst.FuncType),
        LowerFunc(Inst.LowerFunc), MemInst(Inst.MemInst),
        ReallocFunc(Inst.ReallocFunc), PostReturnFunc(Inst.PostReturnFunc),
        CallbackFunc(Inst.CallbackFunc), ParentComp(Inst.ParentComp),
        Enc(Inst.Enc), AsyncLifted(Inst.AsyncLifted),
        AlwaysTaskReturn(Inst.AlwaysTaskReturn),
        HostFunc(std::move(Inst.HostFunc)) {}
  /// Constructor for component native function. `PR` is the optional
  /// post-return core function (CanonicalABI.md L3367-3372); pass nullptr
  /// when the canon lift declared no post-return option.
  FunctionInstance(const AST::Component::FuncType &Type,
                   Runtime::Instance::FunctionInstance *F,
                   Runtime::Instance::MemoryInstance *M,
                   Runtime::Instance::FunctionInstance *R,
                   const Runtime::Instance::ComponentInstance *P,
                   Runtime::Instance::FunctionInstance *PR = nullptr,
                   StringEncoding E = StringEncoding::UTF8) noexcept
      : FuncType(Type), LowerFunc(F), MemInst(M), ReallocFunc(R),
        PostReturnFunc(PR), ParentComp(P), Enc(E) {}
  /// Constructor for a host component function. The function type is owned
  /// by the instance; the callback runs on component-level values.
  FunctionInstance(std::unique_ptr<AST::Component::FuncType> Type,
                   HostFuncCallback &&Callback,
                   const Runtime::Instance::ComponentInstance *P) noexcept
      : OwnedFuncType(std::move(Type)), FuncType(*OwnedFuncType),
        LowerFunc(nullptr), MemInst(nullptr), ReallocFunc(nullptr),
        PostReturnFunc(nullptr), ParentComp(P), Enc(StringEncoding::UTF8),
        HostFunc(std::move(Callback)) {}

  /// Host function accessors.
  bool isHostFunction() const noexcept { return static_cast<bool>(HostFunc); }
  const HostFuncCallback &getHostFunc() const noexcept { return HostFunc; }

  /// Getter for component function type.
  const AST::Component::FuncType &getFuncType() const noexcept {
    return FuncType;
  }

  /// Getter for lower core function instance.
  Runtime::Instance::FunctionInstance *getLowerFunction() const noexcept {
    return LowerFunc;
  }

  /// Getter for memory instance to value conversion.
  Runtime::Instance::MemoryInstance *getMemoryInstance() const noexcept {
    return MemInst;
  }

  /// Getter for allocation core function instance.
  Runtime::Instance::FunctionInstance *getAllocFunction() const noexcept {
    return ReallocFunc;
  }

  /// Getter for the owning component instance. Required for resolving
  /// TypeIndex-based component types through the canonical ABI.
  const Runtime::Instance::ComponentInstance *
  getComponentInstance() const noexcept {
    return ParentComp;
  }

  /// Getter for the post-return core function instance, or nullptr when the
  /// canon lift declared no post-return option (CanonicalABI.md L3367-3372).
  Runtime::Instance::FunctionInstance *getPostReturnFunction() const noexcept {
    return PostReturnFunc;
  }

  /// Getter for the guest string encoding declared by the canon lift's
  /// `string-encoding` option (defaults to UTF-8).
  StringEncoding getStringEncoding() const noexcept { return Enc; }

  /// Async lift options (`async`, `(callback f)`, `always-task-return`).
  void setAsyncOptions(bool Async,
                       Runtime::Instance::FunctionInstance *Callback,
                       bool AlwaysReturn) noexcept {
    AsyncLifted = Async;
    CallbackFunc = Callback;
    AlwaysTaskReturn = AlwaysReturn;
  }
  bool isAsyncLifted() const noexcept { return AsyncLifted; }
  Runtime::Instance::FunctionInstance *getCallbackFunction() const noexcept {
    return CallbackFunc;
  }
  bool isAlwaysTaskReturn() const noexcept { return AlwaysTaskReturn; }

protected:
  std::unique_ptr<AST::Component::FuncType> OwnedFuncType;
  const AST::Component::FuncType &FuncType;
  Runtime::Instance::FunctionInstance *LowerFunc;
  Runtime::Instance::MemoryInstance *MemInst;
  Runtime::Instance::FunctionInstance *ReallocFunc;
  Runtime::Instance::FunctionInstance *PostReturnFunc;
  Runtime::Instance::FunctionInstance *CallbackFunc = nullptr;
  const Runtime::Instance::ComponentInstance *ParentComp;
  StringEncoding Enc;
  bool AsyncLifted = false;
  bool AlwaysTaskReturn = false;
  HostFuncCallback HostFunc;
};

} // namespace Component
} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
