// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#pragma once

#include "ast/component/type.h"
#include "runtime/instance/function.h"
#include "runtime/instance/memory.h"

#include <memory>

namespace WasmEdge {
namespace Runtime {
namespace Instance {
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
        MemInst(Inst.MemInst), ReallocFunc(Inst.ReallocFunc) {}
  /// Constructor for component native function.
  FunctionInstance(const AST::Component::FuncType &Type,
                   Runtime::Instance::FunctionInstance *F,
                   Runtime::Instance::MemoryInstance *M,
                   Runtime::Instance::FunctionInstance *R) noexcept
      : FuncType(Type), LowerFunc(F), MemInst(M), ReallocFunc(R) {}

  /// Getter of component function type.
  const AST::Component::FuncType &getFuncType() const noexcept {
    return FuncType;
  }

  /// Getter of lower core function instance.
  Runtime::Instance::FunctionInstance *getLowerFunction() const noexcept {
    return LowerFunc;
  }

  /// Getter of memory instance to value conversion.
  Runtime::Instance::MemoryInstance *getMemoryInstance() const noexcept {
    return MemInst;
  }

  /// Getter of allocation core function instance.
  Runtime::Instance::FunctionInstance *getAllocFunction() const noexcept {
    return ReallocFunc;
  }

protected:
  const AST::Component::FuncType &FuncType;
  Runtime::Instance::FunctionInstance *LowerFunc;
  Runtime::Instance::MemoryInstance *MemInst;
  Runtime::Instance::FunctionInstance *ReallocFunc;
};

} // namespace Component
} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
