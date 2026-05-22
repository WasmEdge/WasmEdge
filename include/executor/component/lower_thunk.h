// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/executor/component/lower_thunk.h -------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Synthesized core wasm function backing `canon lower` (CanonicalABI.md
/// L3534-3640, sync branch). The thunk is a HostFunctionBase subclass whose
/// run() lifts core wasm args to component values, invokes a wrapped
/// Component::FunctionInstance, and lowers the result back.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "executor/component/canonical_abi.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/component/component.h"
#include "runtime/instance/component/function.h"
#include "runtime/instance/function.h"
#include "runtime/instance/memory.h"

namespace WasmEdge {
namespace Executor {

class Executor;

class CanonLowerHostFunc : public Runtime::HostFunctionBase {
public:
  /// Construct a lower-side thunk. The flat ABI signature (already produced by
  /// `flattenFuncType(IsLift=false)`) determines the core wasm signature
  /// exposed to wasm callers; `Callee` is the component function being
  /// adapted; `Memory` / `Realloc` come from the canon lower options.
  CanonLowerHostFunc(
      Executor *Exec, const CanonicalABI::FlatFuncType &FlatSig,
      Runtime::Instance::Component::FunctionInstance *Callee,
      Runtime::Instance::MemoryInstance *Memory,
      Runtime::Instance::FunctionInstance *Realloc,
      const Runtime::Instance::ComponentInstance *CompInst) noexcept;

  Expect<void> run(const Runtime::CallingFrame &Frame,
                   Span<const ValVariant> Args, Span<ValVariant> Rets) override;

private:
  Executor *Exec;
  Runtime::Instance::Component::FunctionInstance *Callee;
  Runtime::Instance::MemoryInstance *Memory;
  Runtime::Instance::FunctionInstance *Realloc;
  const Runtime::Instance::ComponentInstance *CompInst;
  // Cached at construction: true if lower added a trailing out-pointer
  // (spec L2829-2831: flat_results > MAX_FLAT_RESULTS).
  bool HasOutPtr;
};

} // namespace Executor
} // namespace WasmEdge
