// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors
#pragma once

#include "ast/component/type.h"
#include "runtime/instance/function.h"
#include "runtime/instance/memory.h"

#include <memory>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ComponentInstance; // forward decl for parent component pointer

namespace Component {

class FunctionInstance {
  // The component function instance currently can only be instantiated by the
  // `canon lift` operation. For the component host functions, the extension may
  // be implemented in the future.
public:
  FunctionInstance() = delete;
  /// Move constructor.
  FunctionInstance(FunctionInstance &&Inst) noexcept
      : FuncType(Inst.FuncType), LowerFunc(Inst.LowerFunc),
        MemInst(Inst.MemInst), ReallocFunc(Inst.ReallocFunc),
        PostReturnFunc(Inst.PostReturnFunc), ParentComp(Inst.ParentComp) {}
  /// Constructor for component native function. `PR` is the optional
  /// post-return core function (CanonicalABI.md L3367-3372); pass nullptr
  /// when the canon lift declared no post-return option.
  FunctionInstance(const AST::Component::FuncType &Type,
                   Runtime::Instance::FunctionInstance *F,
                   Runtime::Instance::MemoryInstance *M,
                   Runtime::Instance::FunctionInstance *R,
                   const Runtime::Instance::ComponentInstance *P,
                   Runtime::Instance::FunctionInstance *PR = nullptr) noexcept
      : FuncType(Type), LowerFunc(F), MemInst(M), ReallocFunc(R),
        PostReturnFunc(PR), ParentComp(P) {}

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

protected:
  const AST::Component::FuncType &FuncType;
  Runtime::Instance::FunctionInstance *LowerFunc;
  Runtime::Instance::MemoryInstance *MemInst;
  Runtime::Instance::FunctionInstance *ReallocFunc;
  Runtime::Instance::FunctionInstance *PostReturnFunc;
  const Runtime::Instance::ComponentInstance *ParentComp;
};

} // namespace Component
} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
