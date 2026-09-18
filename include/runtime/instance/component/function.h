// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/component/function.h - Function ---------===//
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
#include "runtime/component/hostfunc.h"
#include "runtime/instance/function.h"
#include "runtime/instance/memory.h"

#include <cstdint>
#include <memory>
#include <utility>

namespace WasmEdge {
namespace Runtime {

namespace Instance {
class ComponentInstance;
} // namespace Instance

namespace Component {

/// The `string-encoding` canon option. The host always holds component
/// strings as UTF-8; this selects the guest's linear-memory layout.
enum class StringEncoding : uint8_t { UTF8, UTF16, Latin1UTF16 };

/// The `canonopt` of the specification, plus the component instance whose
/// type-index space these options are read against.
struct CanonOptions {
  const Instance::ComponentInstance *Inst = nullptr;
  Instance::MemoryInstance *Mem = nullptr;
  Instance::FunctionInstance *Realloc = nullptr;
  Instance::FunctionInstance *PostReturn = nullptr;
  Instance::FunctionInstance *Callback = nullptr;
  /// Guest string encoding; defaults to UTF-8.
  StringEncoding Encoding = StringEncoding::UTF8;
  bool Async = false;

  /// Whether the memory of the options is a 64-bit memory.
  bool isMemory64() const noexcept {
    return Mem != nullptr && Mem->getMemoryType().getLimit().is64();
  }
};

} // namespace Component

namespace Instance {

/// A component function instance comes either from `canon lift` or from a
/// host function over component-level values.
class ComponentFunctionInstance {
public:
  ComponentFunctionInstance() = delete;
  /// Constructor for a component native function lifted from a core function.
  /// The function type reads its type indices against TypeCompInst.
  ComponentFunctionInstance(
      const AST::Component::FuncType &Type,
      const ComponentInstance *TypeCompInst, FunctionInstance *CoreFunc,
      const Runtime::Component::CanonOptions &CanonOpts) noexcept
      : FuncType(Type), TypeInst(TypeCompInst), LowerFunc(CoreFunc),
        Opts(CanonOpts) {}
  /// Constructor for a host component function of a host instance. The host
  /// function owns the function type.
  ComponentFunctionInstance(
      std::unique_ptr<Runtime::Component::HostFunctionBase> &&HostFunc,
      const ComponentInstance *CompInst) noexcept
      : Host(std::move(HostFunc)), FuncType(Host->getFuncType()),
        TypeInst(CompInst), LowerFunc(nullptr), Opts{CompInst} {}

  /// Host function accessors.
  bool isHostFunction() const noexcept { return static_cast<bool>(Host); }
  Runtime::Component::HostFunctionBase &getHostFunc() const noexcept {
    return *Host;
  }

  /// Getter for component function type.
  const AST::Component::FuncType &getFuncType() const noexcept {
    return FuncType;
  }

  /// Getter for the instance the function type reads its type indices against.
  const ComponentInstance *getTypeInstance() const noexcept { return TypeInst; }

  /// Getter for lower core function instance.
  FunctionInstance *getLowerFunction() const noexcept { return LowerFunc; }

  /// Getter for the canonical options of the `canon lift`.
  const Runtime::Component::CanonOptions &getCanonOptions() const noexcept {
    return Opts;
  }

  /// Getter for the owning component instance.
  const ComponentInstance *getComponentInstance() const noexcept {
    return Opts.Inst;
  }

private:
  /// \name Data of component function instance.
  /// @{
  std::unique_ptr<Runtime::Component::HostFunctionBase> Host;
  const AST::Component::FuncType &FuncType;
  const ComponentInstance *TypeInst;
  FunctionInstance *LowerFunc;
  Runtime::Component::CanonOptions Opts;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
