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
#include "runtime/instance/function.h"

#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ComponentInstance;

/// A component function instance comes either from `canon lift` or from an
/// embedder host function over component-level values.
class ComponentFunctionInstance {
public:
  /// Host callback: component-level values in, (value, type) pairs out.
  using HostFuncCallback = std::function<
      Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>(
          Span<const ComponentValVariant>)>;

  ComponentFunctionInstance() = delete;
  /// Move constructor.
  ComponentFunctionInstance(ComponentFunctionInstance &&Inst) noexcept
      : OwnedFuncType(std::move(Inst.OwnedFuncType)),
        FuncType(OwnedFuncType ? *OwnedFuncType : Inst.FuncType),
        LowerFunc(Inst.LowerFunc), Opts(Inst.Opts),
        HostFunc(std::move(Inst.HostFunc)) {}
  /// Constructor for a component native function lifted from the core
  /// function F under the canonical options O.
  ComponentFunctionInstance(const AST::Component::FuncType &Type,
                            FunctionInstance *F,
                            const Runtime::Component::CanonOptions &O) noexcept
      : FuncType(Type), LowerFunc(F), Opts(O) {}
  /// Constructor for a host component function. The instance owns the
  /// function type. The callback runs on component-level values.
  ComponentFunctionInstance(std::unique_ptr<AST::Component::FuncType> Type,
                            HostFuncCallback &&Callback,
                            const ComponentInstance *P) noexcept
      : OwnedFuncType(std::move(Type)), FuncType(*OwnedFuncType),
        LowerFunc(nullptr), Opts{P}, HostFunc(std::move(Callback)) {}

  /// Host function accessors.
  bool isHostFunction() const noexcept { return static_cast<bool>(HostFunc); }
  const HostFuncCallback &getHostFunc() const noexcept { return HostFunc; }

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
  std::unique_ptr<AST::Component::FuncType> OwnedFuncType;
  const AST::Component::FuncType &FuncType;
  FunctionInstance *LowerFunc;
  Runtime::Component::CanonOptions Opts;
  HostFuncCallback HostFunc;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
