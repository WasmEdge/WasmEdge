// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/api/internal/managed_ref_getter.h - shared getter gate -===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file shares the managed-ref predicates of the standalone C APIs
/// between the WasmEdge C-API, the WASM C-API and the GC test suite, so the
/// test asserts against the functions the C-APIs actually call.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"
#include "common/spdlog.h"
#include "common/types.h"
#include "executor/executor.h"

#include <utility>

namespace WasmEdge {

// Resolve canHoldManaged for a standalone C API table or global, which has no
// module type list. Abstract funcref and nullfuncref give false. Every other
// ref type gives true, including a concrete type index, because without the
// type list it can be a struct or array type.
inline bool resolveCanHoldManagedNoTypeList(const ValType &RT) noexcept {
  return RT.isRefType() && !(RT.isFuncRefType() && RT.isAbsHeapType());
}

// True if WasmEdge_GlobalInstanceGetValue or WasmEdge_TableInstanceGetData
// must return a null sentinel instead of the stored value for a slot of type
// \p RT. These getters do not root the reference, so a collection can reclaim
// the object while the host holds it. externref is excluded, because the host
// stores its own pointers through it. funcref already has CanHoldManaged false.
inline bool isRestrictedManagedRefGetterType(bool CanHoldManaged,
                                             const ValType &RT) noexcept {
  return CanHoldManaged && !RT.isExternRefType();
}

// Shared body of WasmEdge_GlobalInstanceGetValueRetained and
// WasmEdge_TableInstanceGetDataRetained. \p Read reads the slot, \p DeclType is
// the slot type, and \p Exec is the executor whose allocator roots the
// reference. The caller has already rejected an instance that another
// executor owns (hasForeignAllocator).
//
// A live struct or array reference is rooted with Allocator::retainResult, as
// Executor::invoke does, and returned with its abstract heap type so that
// WasmEdge_ExecutorReleaseRef can match it. Null, funcref, and externref values
// are returned with the slot type and are not rooted: an externalized GC object
// is released only by releaseAllRefs, so rooting it here would leak a root.
//
// The host thread is not a registered mutator, so no handshake waits for it.
// Between the read and retainResult, a guest could clear the slot and a
// collection could free the object. Thus the read, the type expansion and the
// retain run while this thread holds the controller's exclusive-operation
// token: no collection can start until it is released. OwnedGrowing is the
// only state that a waiter can take (see Controller::beginExclusiveOp); it
// does not stop other mutators.
template <typename ReadSlot>
inline Expect<std::pair<RefVariant, ValType>>
retainManagedRefFromSlot(Executor::Executor &Exec, const ValType &DeclType,
                         ReadSlot &&Read) noexcept {
  struct ExclusiveReadWindow {
    GC::Controller &Ctrl;
    uint64_t Gen = 0;
    bool Held = false;
    explicit ExclusiveReadWindow(GC::Controller &C) noexcept : Ctrl(C) {
      Held = Ctrl.beginExclusiveOp(GC::Controller::ExclusiveState::OwnedGrowing,
                                   Gen);
    }
    ~ExclusiveReadWindow() noexcept {
      if (Held) {
        Ctrl.endExclusiveOp(Gen, GC::Controller::ExclusiveState::OwnedGrowing);
      }
    }
    ExclusiveReadWindow(const ExclusiveReadWindow &) = delete;
    ExclusiveReadWindow &operator=(const ExclusiveReadWindow &) = delete;
  } Window(Exec.getController());
  if (!Window.Held) {
    // Refused only when the controller is closing.
    spdlog::error(ErrCode::Value::Interrupted);
    return Unexpect(ErrCode::Value::Interrupted);
  }
  EXPECTED_TRY(RefVariant Ref, Read());
  if (!DeclType.isExternRefType() && !Ref.isNull()) {
    // Expand a concrete type index (relative to the defining module) to its
    // abstract heap type, as Executor::invoke does.
    const ValType RefType = Exec.expandGCRefType(Ref);
    if (RefType.isHostRetainedRefType()) {
      Exec.getAllocator().retainResult(Ref);
      return std::pair<RefVariant, ValType>(Ref, RefType);
    }
  }
  // Not rooted: return the value with the slot type.
  return std::pair<RefVariant, ValType>(Ref, DeclType);
}

} // namespace WasmEdge
