// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- wasmedge/executor/component/resource_thunk.h ----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Synthesized core wasm functions backing the `canon resource.*` built-ins
/// (CanonicalABI.md "canon resource.new/drop/rep"). Each thunk operates on
/// the defining or using component instance's canonical handle table.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "runtime/hostfunc.h"
#include "runtime/instance/component/component.h"

namespace WasmEdge {
namespace Executor {

class Executor;

using ResourceTypeRT = Runtime::Instance::ComponentInstance::ResourceTypeRT;

/// canon resource.new $rt : [rep:i32] -> [i32]
class CanonResourceNewHostFunc : public Runtime::HostFunctionBase {
public:
  CanonResourceNewHostFunc(const Runtime::Instance::ComponentInstance *Inst,
                           const ResourceTypeRT *RT) noexcept;

  Expect<void> run(const Runtime::CallingFrame &Frame,
                   Span<const ValVariant> Args, Span<ValVariant> Rets) override;

private:
  const Runtime::Instance::ComponentInstance *Inst;
  const ResourceTypeRT *RT;
};

/// canon resource.rep $rt : [i32] -> [rep:i32]
class CanonResourceRepHostFunc : public Runtime::HostFunctionBase {
public:
  CanonResourceRepHostFunc(const Runtime::Instance::ComponentInstance *Inst,
                           const ResourceTypeRT *RT) noexcept;

  Expect<void> run(const Runtime::CallingFrame &Frame,
                   Span<const ValVariant> Args, Span<ValVariant> Rets) override;

private:
  const Runtime::Instance::ComponentInstance *Inst;
  const ResourceTypeRT *RT;
};

/// canon resource.drop $rt : [i32] -> []
class CanonResourceDropHostFunc : public Runtime::HostFunctionBase {
public:
  CanonResourceDropHostFunc(Executor *Exec,
                            const Runtime::Instance::ComponentInstance *Inst,
                            const ResourceTypeRT *RT) noexcept;

  Expect<void> run(const Runtime::CallingFrame &Frame,
                   Span<const ValVariant> Args, Span<ValVariant> Rets) override;

private:
  Executor *Exec;
  const Runtime::Instance::ComponentInstance *Inst;
  const ResourceTypeRT *RT;
};

} // namespace Executor
} // namespace WasmEdge
