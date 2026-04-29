// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#pragma once

#include "ast/component/canonical.h"
#include "ast/component/type.h"
#include "common/types.h"
#include "runtime/instance/function.h"
#include "runtime/instance/memory.h"

#include <memory>
#include <variant>

namespace WasmEdge {
namespace Runtime {
namespace Instance {
namespace Component {

class HostFunctionBase;

/// Canonical ABI options for component functions
struct CanonicalOptions {
  AST::Component::CanonOpt::OptCode StringEncoding =
      AST::Component::CanonOpt::OptCode::Encode_UTF8;
  Runtime::Instance::MemoryInstance *Memory = nullptr;
  Runtime::Instance::FunctionInstance *Realloc = nullptr;
  Runtime::Instance::FunctionInstance *PostReturn = nullptr;
  bool IsAsync = false;

  CanonicalOptions() noexcept = default;
};

/// Custom deleter for HostFunctionBase to avoid incomplete type issues
struct HostFunctionDeleter {
  void operator()(HostFunctionBase *ptr) const;
};

/// Unified component function instance supporting both lifted core functions
/// and native component host functions
class FunctionInstance {
public:
  enum class FunctionKind : uint8_t {
    Lifted, /// Core WASM function lifted via canon lift
    Host,   /// Native component host function
  };

  FunctionInstance() = delete;

  /// Move constructor.
  FunctionInstance(FunctionInstance &&Inst) noexcept
      : FuncType(Inst.FuncType), Kind(Inst.Kind), CanonOpts(Inst.CanonOpts),
        LowerFunc(Inst.LowerFunc), HostFunc(std::move(Inst.HostFunc)) {}

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

  /// Getter of component function type.
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

  /// Getter of lower core function instance (for lifted functions).
  Runtime::Instance::FunctionInstance *getLowerFunction() const noexcept {
    return LowerFunc;
  }

  /// Getter of host function instance (for host functions).
  HostFunctionBase *getHostFunction() const noexcept { return HostFunc.get(); }

  /// Getter of memory instance for canonical ABI conversion.
  Runtime::Instance::MemoryInstance *getMemoryInstance() const noexcept {
    return CanonOpts.Memory;
  }

  /// Getter of realloc function for canonical ABI conversion.
  Runtime::Instance::FunctionInstance *getReallocFunction() const noexcept {
    return CanonOpts.Realloc;
  }

  /// Getter of post-return function for canonical ABI cleanup.
  Runtime::Instance::FunctionInstance *getPostReturnFunction() const noexcept {
    return CanonOpts.PostReturn;
  }

  /// Check if function is async.
  bool isAsync() const noexcept { return CanonOpts.IsAsync; }

protected:
  const AST::Component::FuncType &FuncType;
  FunctionKind Kind;
  CanonicalOptions CanonOpts;

  // For lifted functions
  Runtime::Instance::FunctionInstance *LowerFunc;

  // For host functions
  std::unique_ptr<HostFunctionBase, HostFunctionDeleter> HostFunc;
};

} // namespace Component
} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
