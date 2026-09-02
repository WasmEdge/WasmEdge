// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/component/function.h - Component Function -=//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the component function instance definition in store
/// manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/type.h"
#include "common/types.h"
#include "runtime/component/canonopt.h"
#include "runtime/component/hostfunc.h"
#include "runtime/instance/function.h"

#include <memory>
#include <utility>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ComponentInstance;

/// A component function instance comes either from `canon lift` or from a
/// host function over component-level values.
class ComponentFunctionInstance {
public:
  ComponentFunctionInstance() = delete;
  /// Constructor for a component native function lifted from the core
  /// function F under the canonical options O.
  ComponentFunctionInstance(const AST::Component::FuncType &Type,
                            FunctionInstance *F,
                            const Runtime::Component::CanonOptions &O) noexcept
      : FuncType(Type), LowerFunc(F), Opts(O) {}
  /// Constructor for a host component function of the host instance P. The
  /// host function owns the function type.
  ComponentFunctionInstance(
      std::unique_ptr<Runtime::Component::HostFunctionBase> &&H,
      const ComponentInstance *P) noexcept
      : Host(std::move(H)), FuncType(Host->getFuncType()), LowerFunc(nullptr),
        Opts{P} {}

  /// Host function accessors.
  bool isHostFunction() const noexcept { return static_cast<bool>(Host); }
  Runtime::Component::HostFunctionBase &getHostFunc() const noexcept {
    return *Host;
  }

  /// Getter for component function type.
  const AST::Component::FuncType &getFuncType() const noexcept {
    return FuncType;
  }

  /// Getter for lower core function instance.
  FunctionInstance *getLowerFunction() const noexcept { return LowerFunc; }

  /// Getter for the canonical options of the `canon lift`.
  const Runtime::Component::CanonOptions &getCanonOptions() const noexcept {
    return Opts;
  }

  /// Getter for the owning component instance, whose type-index space the
  /// function type is read against.
  const ComponentInstance *getComponentInstance() const noexcept {
    return Opts.Inst;
  }

private:
  /// \name Data of component function instance.
  /// @{
  std::unique_ptr<Runtime::Component::HostFunctionBase> Host;
  const AST::Component::FuncType &FuncType;
  FunctionInstance *LowerFunc;
  Runtime::Component::CanonOptions Opts;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
