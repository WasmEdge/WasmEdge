// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#pragma once

#include "ast/component/type.h"
#include "common/types.h"
#include "runtime/instance/function.h"
#include "runtime/instance/memory.h"

#include <memory>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ComponentInstance;

namespace Component {

class HostFunctionBase;

/// Canonical ABI options for component functions.
struct CanonicalOptions {
  Runtime::Instance::MemoryInstance *Memory = nullptr;
  Runtime::Instance::FunctionInstance *Realloc = nullptr;
  Runtime::Instance::FunctionInstance *PostReturn = nullptr;
  const Runtime::Instance::ComponentInstance *ParentComp = nullptr;
  StringEncoding StringEnc = StringEncoding::UTF8;
  bool IsAsync = false;

  CanonicalOptions() noexcept = default;
};

/// Custom deleter for HostFunctionBase to avoid incomplete type issues.
struct HostFunctionDeleter {
  void operator()(HostFunctionBase *Ptr) const;
};

/// Unified component function instance supporting both lifted core functions
/// and native component host functions.
class FunctionInstance {
public:
  enum class FunctionKind : uint8_t {
    Lifted,
    Host,
  };

  FunctionInstance() = delete;
  ~FunctionInstance();
  FunctionInstance(FunctionInstance &&Inst) noexcept;
  FunctionInstance &operator=(FunctionInstance &&Inst) noexcept;

  /// Constructor for lifted component function (from core WASM).
  FunctionInstance(const AST::Component::FuncType &Type,
                   Runtime::Instance::FunctionInstance *F,
                   const CanonicalOptions &Opts) noexcept
      : FuncType(Type), Kind(FunctionKind::Lifted), CanonOpts(Opts),
        LowerFunc(F), HostFunc(nullptr) {}

  /// Constructor for native component host function.
  FunctionInstance(const AST::Component::FuncType &Type,
                   std::unique_ptr<HostFunctionBase, HostFunctionDeleter> &&HF,
                   const CanonicalOptions &Opts = CanonicalOptions()) noexcept
      : FuncType(Type), Kind(FunctionKind::Host), CanonOpts(Opts),
        LowerFunc(nullptr), HostFunc(std::move(HF)) {}

  /// Getter for component function type.
  const AST::Component::FuncType &getFuncType() const noexcept {
    return FuncType;
  }

  /// Getter of function kind.
  FunctionKind getKind() const noexcept { return Kind; }

  /// Check if this is a lifted function.
  bool isLifted() const noexcept { return Kind == FunctionKind::Lifted; }

  /// Check if this is a host function.
  bool isHost() const noexcept { return Kind == FunctionKind::Host; }

  /// Getter of canonical ABI options.
  const CanonicalOptions &getCanonicalOptions() const noexcept {
    return CanonOpts;
  }

  /// Getter for lower core function instance (for lifted functions).
  Runtime::Instance::FunctionInstance *getLowerFunction() const noexcept {
    return LowerFunc;
  }

  /// Getter of host function instance (for host functions).
  HostFunctionBase *getHostFunction() const noexcept { return HostFunc.get(); }

  /// Getter for memory instance for canonical ABI conversion.
  Runtime::Instance::MemoryInstance *getMemoryInstance() const noexcept {
    return CanonOpts.Memory;
  }

  /// Getter for realloc function for canonical ABI conversion.
  Runtime::Instance::FunctionInstance *getAllocFunction() const noexcept {
    return CanonOpts.Realloc;
  }

  /// Getter for realloc function for canonical ABI conversion.
  Runtime::Instance::FunctionInstance *getReallocFunction() const noexcept {
    return CanonOpts.Realloc;
  }

  /// Getter for the owning component instance.
  const Runtime::Instance::ComponentInstance *
  getComponentInstance() const noexcept {
    return CanonOpts.ParentComp;
  }

  /// Getter for the post-return core function instance.
  Runtime::Instance::FunctionInstance *getPostReturnFunction() const noexcept {
    return CanonOpts.PostReturn;
  }

  /// Getter for the guest string encoding declared by the canon lift option.
  StringEncoding getStringEncoding() const noexcept {
    return CanonOpts.StringEnc;
  }

  /// Check if function is async.
  bool isAsync() const noexcept { return CanonOpts.IsAsync; }

private:
  AST::Component::FuncType FuncType;
  FunctionKind Kind;
  CanonicalOptions CanonOpts;

  Runtime::Instance::FunctionInstance *LowerFunc;
  std::unique_ptr<HostFunctionBase, HostFunctionDeleter> HostFunc;
};

} // namespace Component
} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
