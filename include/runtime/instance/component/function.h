// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors
#pragma once

#include "ast/component/type.h"
#include "runtime/component/canonopt.h"
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
        LowerFunc(Inst.LowerFunc), Opts(Inst.Opts),
        HostFunc(std::move(Inst.HostFunc)) {}
  /// Constructor for a component native function lifted from the core
  /// function F under the canonical options O.
  FunctionInstance(const AST::Component::FuncType &Type,
                   Runtime::Instance::FunctionInstance *F,
                   const Runtime::Component::CanonOptions &O) noexcept
      : FuncType(Type), LowerFunc(F), Opts(O) {}
  /// Constructor for a host component function. The function type is owned
  /// by the instance; the callback runs on component-level values.
  FunctionInstance(std::unique_ptr<AST::Component::FuncType> Type,
                   HostFuncCallback &&Callback,
                   const Runtime::Instance::ComponentInstance *P) noexcept
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
  Runtime::Instance::FunctionInstance *getLowerFunction() const noexcept {
    return LowerFunc;
  }

  /// Getter for memory instance to value conversion.
  Runtime::Instance::MemoryInstance *getMemoryInstance() const noexcept {
    return Opts.Mem;
  }

  /// Getter for allocation core function instance.
  Runtime::Instance::FunctionInstance *getAllocFunction() const noexcept {
    return Opts.Realloc;
  }

  /// Getter for the owning component instance. Required for resolving
  /// TypeIndex-based component types through the canonical ABI.
  const Runtime::Instance::ComponentInstance *
  getComponentInstance() const noexcept {
    return Opts.Inst;
  }

  /// Getter for the post-return core function instance, or nullptr when the
  /// canon lift declared no post-return option (CanonicalABI.md L3367-3372).
  Runtime::Instance::FunctionInstance *getPostReturnFunction() const noexcept {
    return Opts.PostReturn;
  }

  /// Getter for the guest string encoding declared by the canon lift's
  /// `string-encoding` option (defaults to UTF-8).
  StringEncoding getStringEncoding() const noexcept { return Opts.Enc; }

  /// Getter for the canonical options this function was lifted under.
  const Runtime::Component::CanonOptions &getCanonOptions() const noexcept {
    return Opts;
  }

protected:
  std::unique_ptr<AST::Component::FuncType> OwnedFuncType;
  const AST::Component::FuncType &FuncType;
  Runtime::Instance::FunctionInstance *LowerFunc;
  Runtime::Component::CanonOptions Opts;
  HostFuncCallback HostFunc;
};

} // namespace Component
} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
