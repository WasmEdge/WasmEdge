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

  /// Register this global with the GC allocator as a scanned root.
  ///
  /// One-shot: must be called at most once, before any other allocator has
  /// been attached (precondition enforced only by the debug assuming() below;
  /// it is a no-op in NDEBUG builds). Registers this instance via addGlobal()
  /// so the collector can scan and update its Value; the instance must stay
  /// valid until the matching removeGlobal() (see clearAllocator()).
  void setAllocator(GC::Allocator &A) noexcept {
    assuming(Allocator == nullptr);
    Allocator = &A;
    Allocator->addGlobal(*this);
  }

  /// Getter for global type.
  const AST::GlobalType &getGlobalType() const noexcept { return GlobType; }

  /// Getter for value.
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

  /// Get a raw, unbarriered pointer to the stored value.
  ///
  /// This is the AOT fast path only: compiled global.get/global.set load and
  /// store through this address directly (see compiler.cpp). Runtime mutation
  /// must instead go through setValue(), which runs the GC write barrier;
  /// storing a reference through this pointer bypasses that barrier and is
  /// sound only under the SATB + conservative-stack-scan reasoning the AOT
  /// path relies on.
  ValVariant *getAddress() noexcept { return &Value; }

private:
  friend class GC::Allocator;
  /// Detach this global from the allocator during allocator teardown.
  ///
  /// Lifetime/teardown contract: once setAllocator() registers this instance
  /// via addGlobal(), the allocator may dereference this GlobalInstance until a
  /// matching removeGlobal() (run from the destructor) or clearAllocator(). The
  /// Allocator pointer is read without synchronization on the destructor and
  /// barrier paths, so teardown of the global and of its allocator must be
  /// single-threaded with respect to each other (no concurrent teardown):
  /// either ~GlobalInstance removes the registration before ~Allocator, or
  /// ~Allocator calls clearAllocator() under its heap lock first.
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
