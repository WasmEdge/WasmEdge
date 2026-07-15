// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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

  // Rooted by address: a copy/move would leave the new object unregistered and
  // its destructor's removeGlobal pointing at an address never stored.
  GlobalInstance(const GlobalInstance &) = delete;
  GlobalInstance(GlobalInstance &&) = delete;
  GlobalInstance &operator=(const GlobalInstance &) = delete;
  GlobalInstance &operator=(GlobalInstance &&) = delete;

  /// Attach the GC allocator that scans and updates this global's Value.
  /// Attach-once, with the same import/cross-executor/host rules as
  /// TableInstance::setAllocator: first allocator wins, a regularly
  /// instantiated global must not re-register, a host global is attached only
  /// when imported. The instance must stay valid until the matching
  /// removeGlobal().
  void setAllocator(GC::Allocator &A) noexcept {
    if (Allocator != nullptr) {
      return;
    }
    Allocator = &A;
    Allocator->addGlobal(*this);
  }

  /// True if attached to an allocator other than A. Import uses this to reject
  /// sharing a GC ref-typed global across executors (see setAllocator); interim
  /// guard until cross-allocator roots exist.
  bool hasForeignAllocator(const GC::Allocator &A) const noexcept {
    return Allocator != nullptr && Allocator != &A;
  }

  /// Getter for global type.
  const AST::GlobalType &getGlobalType() const noexcept { return GlobType; }

  /// Getter for value. Returns a BORROWED reference: a GC ref stays alive only
  /// while stored here (a scanned root). Unlike function returns, getter APIs
  /// (WasmEdge_GlobalInstanceGetValue) do not add it to HostRoots, so a later
  /// setValue + collection can reclaim it while the host still holds the value.
  ValVariant getValue() const noexcept { return Value; }

  /// Setter for value.
  void setValue(const ValVariant &Val) noexcept {
    // The global is a scanned GC root; shade the overwritten and newly stored
    // reference so a concurrent collection does not miss an object reachable
    // only through this global after the root snapshot (as for struct/array
    // field writes).
    if (Allocator) {
      Allocator->writeBarrier(Value);
      Allocator->writeBarrier(Val);
    }
    Value = Val;
  }

  /// Get a raw, unbarriered pointer to the stored value.
  ///
  /// AOT fast path only: compiled global.get/global.set load/store through this
  /// address directly (see compiler.cpp); other mutation must use setValue()
  /// for its barrier. The compiled global.set instead calls the kWriteBarrier
  /// intrinsic (proxyWriteBarrier) to reproduce setValue()'s shading, which
  /// with the conservative native-stack scan keeps the direct store sound.
  ValVariant *getAddress() noexcept { return &Value; }

private:
  friend class GC::Allocator;
  /// Detach this global from the allocator during allocator teardown.
  ///
  /// The Allocator pointer is read unsynchronized on the destructor and barrier
  /// paths, so global and allocator teardown must be single-threaded w.r.t.
  /// each other: either ~GlobalInstance removes the registration before
  /// ~Allocator, or ~Allocator calls clearAllocator() under its heap lock
  /// first.
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
