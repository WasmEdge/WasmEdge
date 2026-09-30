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
  // \p CanHoldManagedIn tells whether this global can hold a managed GC
  // object. The caller resolves it, as for TableInstance.
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

  /// Attach the GC allocator that scans and updates this global's Value. Same
  /// rule as TableInstance::setAllocator, but a global never grows, so a
  /// different allocator is rejected only when canHoldManaged(). The instance
  /// must stay valid until the matching removeGlobal().
  Expect<void> setAllocator(GC::Allocator &A) noexcept {
    std::lock_guard<std::mutex> Lock(AttachMutex);
    return setAllocatorLocked(A);
  }

  /// See TableInstance::claimForRegister.
  Expect<bool> claimForRegister(GC::Allocator &A) noexcept {
    std::lock_guard<std::mutex> Lock(AttachMutex);
    EXPECTED_TRY(setAllocatorLocked(A));
    return Allocator == &A;
  }

  /// See TableInstance::detachAllocator.
  void detachAllocator(GC::Allocator &A) noexcept {
    std::lock_guard<std::mutex> Lock(AttachMutex);
    if (Allocator != &A) {
      return;
    }
    assuming(AttachRefs > 0);
    if (--AttachRefs == 0) {
      Allocator->removeGlobal(*this);
      Allocator = nullptr;
    }
  }

  /// True if this global's value type can hold a GC-managed heap object.
  /// Resolved once at construction; see the constructor.
  bool canHoldManaged() const noexcept { return CanHoldManaged; }

  /// True if attached to an allocator other than A. Import uses this to reject
  /// a global that another executor owns (see setAllocator for the rule).
  bool hasForeignAllocator(const GC::Allocator &A) const noexcept {
    return Allocator != nullptr && Allocator != &A;
  }

  /// True if a GC controller owns this global and its value type can hold an
  /// internal GC-managed object. WasmEdge_GlobalInstanceSetValue rejects such
  /// a global. Same rule and externref exclusion as
  /// TableInstance::isManagedByController.
  bool isManagedByController() const noexcept {
    return Allocator != nullptr && Allocator->getController() != nullptr &&
           CanHoldManaged && !GlobType.getValType().isExternRefType();
  }

  /// Getter for global type.
  const AST::GlobalType &getGlobalType() const noexcept { return GlobType; }

  /// Getter for value. Returns a coherent copy of the slot. A GC ref in the
  /// copy is borrowed: it points into the GC heap and is not rooted, so it
  /// stays alive only while this global still holds it.
  ValVariant getValue() const noexcept {
    // Read a reference as one coherent (type, pointer) pair, so a concurrent
    // setValue cannot give a torn pair. Numeric globals use a plain read.
    if (GlobType.getValType().isRefType()) {
      return GC::loadCoherent(Value);
    }
    return Value;
  }

  /// Setter for value.
  void setValue(const ValVariant &Val) noexcept {
    // The global is a GC root; shade the old and the new reference so a
    // concurrent collection does not miss an object reachable only through it.
    if (Allocator) {
      Allocator->writeBarrier(Value);
      Allocator->writeBarrier(Val);
    }
    // Publish a reference as one coherent pair, so the marker and concurrent
    // readers never see a torn slot. Numeric globals use a plain store.
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

  /// The owner-attach rule of setAllocator(). The caller holds AttachMutex.
  Expect<void> setAllocatorLocked(GC::Allocator &A) noexcept {
    if (Allocator == &A) {
      ++AttachRefs;
      return {};
    }
    if (hasForeignAllocator(A)) {
      if (CanHoldManaged) {
        return Unexpect(ErrCode::Value::IncompatibleImportType);
      }
      return {};
    }
    Allocator = &A;
    ++AttachRefs;
    Allocator->addGlobal(*this);
    return {};
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
      AttachRefs = 0;
    }
  }

  /// \name Data of global instance.
  /// @{
  GC::Allocator *Allocator = nullptr;
  // How many holds keep this global attached to Allocator. See
  // TableInstance::AttachRefs.
  uint64_t AttachRefs = 0;
  // See TableInstance::AttachMutex.
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
