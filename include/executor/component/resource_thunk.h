// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- wasmedge/executor/component/resource_thunk.h ----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Synthesized core wasm functions backing the `canon resource.*` built-ins,
/// each operating on a component instance's canonical handle table.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "runtime/hostfunc.h"
#include "runtime/instance/component/component.h"

namespace WasmEdge {
namespace Executor {
namespace Component {

class Executor;

/// canon resource.new $rt : [rep:i32] -> [i32]
class CanonResourceNewHostFunc : public Runtime::HostFunctionBase {
public:
  CanonResourceNewHostFunc(
      const Runtime::Instance::ComponentInstance *Inst,
      const Runtime::Instance::Component::ResourceTypeRT *RT) noexcept;

  Expect<void> run(const Runtime::CallingFrame &Frame,
                   Span<const ValVariant> Args, Span<ValVariant> Rets) override;

private:
  const Runtime::Instance::ComponentInstance *Inst;
  const Runtime::Instance::Component::ResourceTypeRT *RT;
};

/// canon resource.rep $rt : [i32] -> [rep:i32]
class CanonResourceRepHostFunc : public Runtime::HostFunctionBase {
public:
  CanonResourceRepHostFunc(
      const Runtime::Instance::ComponentInstance *Inst,
      const Runtime::Instance::Component::ResourceTypeRT *RT) noexcept;

  Expect<void> run(const Runtime::CallingFrame &Frame,
                   Span<const ValVariant> Args, Span<ValVariant> Rets) override;

private:
  const Runtime::Instance::ComponentInstance *Inst;
  const Runtime::Instance::Component::ResourceTypeRT *RT;
};

/// canon resource.drop $rt : [i32] -> []
class CanonResourceDropHostFunc : public Runtime::HostFunctionBase {
public:
  CanonResourceDropHostFunc(
      Executor *Exec, const Runtime::Instance::ComponentInstance *Inst,
      const Runtime::Instance::Component::ResourceTypeRT *RT) noexcept;

  Expect<void> run(const Runtime::CallingFrame &Frame,
                   Span<const ValVariant> Args, Span<ValVariant> Rets) override;

private:
  Executor *Exec;
  const Runtime::Instance::ComponentInstance *Inst;
  const Runtime::Instance::Component::ResourceTypeRT *RT;
};

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
