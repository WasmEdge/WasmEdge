// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/api/internal/managed_ref_getter.h - shared getter gate -===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file shares the E4.0 legacy managed-ref getter restriction
/// predicate between the C-API implementation (lib/api/wasmedge.cpp) and the
/// GC test suite (test/gc/GCTest.cpp), so the test asserts against the same
/// function the C-API actually calls instead of a hand-rederived copy of
/// the boolean.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/types.h"

namespace WasmEdge {

// E4.0 (spec 4.4): true iff \p RT is a slot type whose legacy BORROWED getter
// (WasmEdge_GlobalInstanceGetValue / WasmEdge_TableInstanceGetData) must be
// restricted to a typed-null sentinel instead of handing out the live value:
// \p CanHoldManaged (the slot's resolved, construction-time
// GlobalInstance::canHoldManaged() / TableInstance::canHoldManaged() bit)
// means the slot MAY hold a GC-managed object, and these getters do not add
// the returned reference to HostRoots (see GlobalInstance::getValue's
// doc-comment) -- so a concurrent collection can reclaim the object while the
// host still holds the bare WasmEdge_Value the getter handed back.
//
// externref is deliberately EXCLUDED even though canHoldManaged() is true for
// it (extern.convert_any can wrap a GC object): it is the host-facing
// reference primitive, and round-tripping a host pointer through it
// (WasmEdge_ValueGenExternRef / WasmEdge_ValueGetExternRef) is the existing,
// shipped C-API workflow (see APIUnitTest.cpp's externref table get/set
// round trip), which must keep working. This mirrors
// TableInstance::isManagedByController's externref exclusion from the raw
// C-API mutation gate (E2.2). funcref never holds a managed object
// (CanHoldManaged is already false for it, resolved at construction), so it
// needs no separate exclusion here.
//
// Restricted internal GC-ref types therefore are: any/eq/i31/struct/array/exn
// and a concrete type index resolving to a struct/array composite.
inline bool isRestrictedManagedRefGetterType(bool CanHoldManaged,
                                             const ValType &RT) noexcept {
  return CanHoldManaged && !RT.isExternRefType();
}

} // namespace WasmEdge
