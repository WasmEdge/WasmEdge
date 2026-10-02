// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/component/function.h - Function ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the component function instance definition.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/type.h"
#include "common/errcode.h"
#include "common/spdlog.h"
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

/// The `string-encoding` canon option.
enum class StringEncoding : uint8_t { UTF8, UTF16, Latin1UTF16 };

/// The `canonopt` of the specification, plus the instance they belong to.
struct CanonOptions {
  Instance::ComponentInstance *Inst = nullptr;
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
  /// The memory of the options; a trap when the options name none.
  Expect<Instance::MemoryInstance *> getMemory() const noexcept {
    if (unlikely(Mem == nullptr)) {
      using namespace std::literals;
      spdlog::error(ErrCode::Value::ComponentTrap);
      spdlog::error("    the canonical option `memory` is required"sv);
      return Unexpect(ErrCode::Value::ComponentTrap);
    }
    return Mem;
  }
  /// A pointer as a flat value of the address type of the memory, and back.
  ValVariant newFlatPtr(uint64_t Ptr) const noexcept {
    return isMemory64() ? ValVariant(Ptr)
                        : ValVariant(static_cast<uint32_t>(Ptr));
  }
  uint64_t getFlatPtr(const ValVariant &Val) const noexcept {
    return isMemory64() ? Val.get<uint64_t>()
                        : static_cast<uint64_t>(Val.get<uint32_t>());
  }
};

} // namespace Component

namespace Instance {

/// A component function instance from `canon lift` or a host function.
class ComponentFunctionInstance {
public:
  ComponentFunctionInstance() = delete;
  /// Constructor for a component function lifted from a core function.
  ComponentFunctionInstance(
      const AST::Component::FuncType &Type,
      const ComponentInstance *TypeCompInst, FunctionInstance *CoreFunc,
      const Runtime::Component::CanonOptions &CanonOpts) noexcept
      : FuncType(Type), TypeInst(TypeCompInst), LowerFunc(CoreFunc),
        Opts(CanonOpts) {}
  /// Constructor for a host component function of a host instance.
  ComponentFunctionInstance(
      std::unique_ptr<Runtime::Component::HostFunctionBase> &&HostFunc,
      ComponentInstance *CompInst) noexcept
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

  /// Getter for the instance the function type reads against.
  const ComponentInstance *getTypeInstance() const noexcept { return TypeInst; }

  /// Getter for lower core function instance.
  FunctionInstance *getLowerFunction() const noexcept { return LowerFunc; }

  /// Getter for the canonical options of the `canon lift`.
  const Runtime::Component::CanonOptions &getCanonOptions() const noexcept {
    return Opts;
  }

  /// Getter for the owning component instance.
  ComponentInstance *getComponentInstance() const noexcept { return Opts.Inst; }

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
