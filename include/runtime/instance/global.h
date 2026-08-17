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
#include "common/errcode.h"
#include "gc/allocator.h"

#include <mutex>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class GlobalInstance {
public:
  GlobalInstance() = delete;
  // \p CanHoldManagedIn is the resolved, immutable "can this global hold a
  // managed GC object" bit, mirroring TableInstance. It cannot be
  // computed here: a concrete heap-type index is defining-module-relative and
  // the constructor has no type list. Callers with a type list (addGlobal)
  // resolve it via AST::TypeMatcher::refTypeCanHoldGCObject; callers without
  // one (e.g. the standalone C API) resolve conservatively -- see those call
  // sites.
  GlobalInstance(const AST::GlobalType &GType, bool CanHoldManagedIn,
                 ValVariant Val = uint128_t(0U)) noexcept
      : GlobType(GType), Value(Val), CanHoldManaged(CanHoldManagedIn) {
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

  /// Attach the GC allocator that scans and updates this global's Value. The
  /// single, fallible, centralized owner-attach check, mirroring
  /// TableInstance::setAllocator. A global is never growable, so only the
  /// managed-capable hazard applies: a second, foreign controller is rejected
  /// when this global can hold a managed GC object, since a ref stored here
  /// would barrier against the wrong allocator and could be swept while still
  /// reachable. A non-managed global is freely shareable: a foreign attach
  /// keeps the first owner and succeeds, and a same-owner re-attach is an
  /// idempotent success. Claiming an unattached global runs under a
  /// per-instance mutex so two controllers cannot race it into a torn owner.
  /// The instance must stay valid until the matching removeGlobal().
  Expect<void> setAllocator(GC::Allocator &A) noexcept {
    std::lock_guard<std::mutex> Lock(AttachMutex);
    auto Res = setAllocatorLocked(A);
    if (!Res) {
      return Unexpect(Res.error());
    }
    return {};
  }

  /// registerModule ownership walk: attach to \p A exactly as setAllocator,
  /// reporting whether THIS call newly claimed ownership so a
  /// preflight-then-commit registration can reverse exactly the attaches it
  /// made if a later root in the same module is rejected. Mirrors
  /// TableInstance::claimForRegister.
  Expect<bool> claimForRegister(GC::Allocator &A) noexcept {
    std::lock_guard<std::mutex> Lock(AttachMutex);
    return setAllocatorLocked(A);
  }

  /// Reverse a claimForRegister()/setAllocator() ownership claim made by
  /// \p A, undoing `Allocator = &A; addGlobal(*this)` -- un-register from A's
  /// root set and clear ownership under AttachMutex. A call naming a non-owner
  /// is a no-op. Distinct from the private clearAllocator() teardown hook.
  void detachAllocator(GC::Allocator &A) noexcept {
    std::lock_guard<std::mutex> Lock(AttachMutex);
    if (Allocator == &A) {
      Allocator->removeGlobal(*this);
      Allocator = nullptr;
    }
  }

  /// True if this global's value type can hold a GC-managed heap object.
  /// Resolved once at construction; see the constructor.
  bool canHoldManaged() const noexcept { return CanHoldManaged; }

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
  ValVariant getValue() const noexcept {
    // Reference-typed global: read the 128-bit (type, pointer) slot as one
    // coherent transaction so a concurrent setValue on another mutator can
    // never hand back a torn pair. Numeric globals cannot form an invalid
    // pointer and keep the plain read.
    if (GlobType.getValType().isRefType()) {
      return GC::loadCoherent(Value);
    }
    return Value;
  }

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
    // Reference-typed global: publish the new (type, pointer) pair atomically
    // so the marker's relaxed pointer-word load and a concurrent coherent
    // reader never observe a torn slot. Numeric globals keep the plain store
    // (no pointer word for the collector to misread).
    if (GlobType.getValType().isRefType()) {
      GC::storeCoherent(Value, Val);
    } else {
      Value = Val;
    }
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

  /// Core of the fallible owner-attach check, assuming the caller holds
  /// AttachMutex. Returns true iff this call newly claimed ownership; false for
  /// an idempotent same-owner or foreign-but-shareable attach; and the
  /// managed-capable error for a rejected foreign attach. A global is never
  /// growable, so only the managed-capable hazard applies. Single source of the
  /// attach rule shared by setAllocator/claimForRegister.
  Expect<bool> setAllocatorLocked(GC::Allocator &A) noexcept {
    if (Allocator == &A) {
      return false;
    }
    if (hasForeignAllocator(A)) {
      if (CanHoldManaged) {
        return Unexpect(ErrCode::Value::IncompatibleImportType);
      }
      return false;
    }
    Allocator = &A;
    Allocator->addGlobal(*this);
    return true;
  }

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
  // Serializes owner claims in setAllocator so two controllers can never race
  // an unattached global into a torn owner. See TableInstance::AttachMutex.
  std::mutex AttachMutex;
  AST::GlobalType GlobType;
  alignas(16) ValVariant Value;
  // Resolved once at construction; see canHoldManaged().
  const bool CanHoldManaged;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
