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
/// `canon_lower`), covering the sync and async lower directions over both
/// sync- and async-typed callees. The thunk lifts core wasm args to
/// component values, drives the callee task through the async runtime, and
/// lowers the result back (immediately for resolved calls, on resolve for
/// pending subtasks).
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
  CanonLowerHostFunc(Executor *Exec, const CanonicalABI::FlatFuncType &FlatSig,
                     Runtime::Instance::Component::FunctionInstance *Callee,
                     Runtime::Instance::MemoryInstance *Memory,
                     Runtime::Instance::FunctionInstance *Realloc,
                     const Runtime::Instance::ComponentInstance *CompInst,
                     StringEncoding Enc = StringEncoding::UTF8,
                     bool AsyncLower = false) noexcept;

  Expect<void> run(const Runtime::CallingFrame &Frame,
                   Span<const ValVariant> Args, Span<ValVariant> Rets) override;

private:
  Executor *Exec;
  Runtime::Instance::Component::FunctionInstance *Callee;
  Runtime::Instance::MemoryInstance *Memory;
  Runtime::Instance::FunctionInstance *Realloc;
  const Runtime::Instance::ComponentInstance *CompInst;
  // Cached at construction: true if the signature carries a trailing
  // out-pointer (sync: flat_results > MAX_FLAT_RESULTS; async: any result).
  bool HasOutPtr;
  // Number of leading flat argument slots holding the lowered parameters.
  uint32_t ParamSlotCount;
  // Guest string encoding from the canon lower `string-encoding` option.
  StringEncoding Enc;
  // The `async` canonical option.
  bool AsyncLower;
};

} // namespace Executor
} // namespace WasmEdge
