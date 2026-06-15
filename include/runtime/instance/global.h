// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/runtime/instance/global.h - Global Instance definition ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the global instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/type.h"
#include "gc/allocator.h"

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class GlobalInstance {
public:
  GlobalInstance() = delete;
  GlobalInstance(const AST::GlobalType &GType,
                 ValVariant Val = uint128_t(0U)) noexcept
      : GlobType(GType), Value(Val) {
    assuming(GType.getValType().isNumType() ||
             GType.getValType().isNullableRefType() ||
             !Val.get<RefVariant>().isNull());
  }

  ~GlobalInstance() noexcept {
    if (Allocator) {
      Allocator->removeGlobal(*this);
    }
  }

  // Registered as a GC root by address; a copy/move would leave the new object
  // unregistered (its value invisible to the collector) and make the
  // destructor's removeGlobal target an address the allocator never stored.
  GlobalInstance(const GlobalInstance &) = delete;
  GlobalInstance(GlobalInstance &&) = delete;
  GlobalInstance &operator=(const GlobalInstance &) = delete;
  GlobalInstance &operator=(GlobalInstance &&) = delete;

  void setAllocator(GC::Allocator &A) noexcept {
    assuming(Allocator == nullptr);
    Allocator = &A;
    Allocator->addGlobal(*this);
  }

  /// Getter of global type.
  const AST::GlobalType &getGlobalType() const noexcept { return GlobType; }

  /// Getter of value.
  ValVariant getValue() const noexcept { return Value; }

  /// Setter for value.
  void setValue(const ValVariant &Val) noexcept {
    // The global is a GC root scanned during marking. Shade both the
    // overwritten and the newly stored reference so a concurrent collection
    // does not miss an object that becomes reachable only through this global
    // after the root snapshot (matches the struct/array field write barriers).
    if (Allocator) {
      Allocator->writeBarrier(Value);
      Allocator->writeBarrier(Val);
    }
    Value = Val;
  }

  ValVariant *getAddress() noexcept { return &Value; }

private:
  friend class GC::Allocator;
  void clearAllocator(GC::Allocator &A) noexcept {
    if (Allocator == &A) {
      Allocator = nullptr;
    }
  }

  /// \name Data of global instance.
  /// @{
  GC::Allocator *Allocator = nullptr;
  AST::GlobalType GlobType;
  alignas(16) ValVariant Value;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
