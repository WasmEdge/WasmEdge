// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/executor/component/lower_thunk.h -------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Synthesized core wasm function backing `canon lower`: it lifts core wasm
/// args, invokes the wrapped component function, and lowers the result back.
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
namespace Component {

class Executor;

class CanonLowerHostFunc : public Runtime::HostFunctionBase {
public:
  /// Construct a lower-side thunk. `FlatSig` is the core wasm signature
  /// exposed to callers; `Memory` / `Realloc` come from the canon options.
  CanonLowerHostFunc(Executor *Exec, const CanonicalABI::FlatFuncType &FlatSig,
                     Runtime::Instance::Component::FunctionInstance *Callee,
                     Runtime::Instance::MemoryInstance *Memory,
                     Runtime::Instance::FunctionInstance *Realloc,
                     const Runtime::Instance::ComponentInstance *CompInst,
                     StringEncoding Enc = StringEncoding::UTF8) noexcept;

  Expect<void> run(const Runtime::CallingFrame &Frame,
                   Span<const ValVariant> Args, Span<ValVariant> Rets) override;

private:
  Executor *Exec;
  Runtime::Instance::Component::FunctionInstance *Callee;
  Runtime::Instance::MemoryInstance *Memory;
  Runtime::Instance::FunctionInstance *Realloc;
  const Runtime::Instance::ComponentInstance *CompInst;
  // Cached at construction: true if the signature carries a trailing
  // out-pointer, which lower adds when flat_results exceeds MaxFlatResults.
  bool HasOutPtr;
  // Guest string encoding from the canon lower `string-encoding` option.
  StringEncoding Enc;
};

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
