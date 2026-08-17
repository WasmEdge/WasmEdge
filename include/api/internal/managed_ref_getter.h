// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/api/internal/managed_ref_getter.h - shared getter gate -===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file shares the legacy managed-ref getter restriction predicate
/// between the C-API implementation and the GC test suite, so the test
/// asserts against the function the C-API actually calls.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/types.h"

namespace WasmEdge {

// True iff \p RT is a slot type whose legacy borrowed getter
// (WasmEdge_GlobalInstanceGetValue / WasmEdge_TableInstanceGetData) must
// return a typed-null sentinel instead of the live value. \p CanHoldManaged is
// the slot's construction-time canHoldManaged() bit; these getters do not add
// the reference to HostRoots, so a concurrent collection could reclaim the
// object while the host still holds the returned WasmEdge_Value.
//
// externref is excluded even though canHoldManaged() is true for it: it is the
// host-facing reference primitive, and round-tripping a host pointer through
// it is a shipped C-API workflow. funcref never holds a managed object, so
// CanHoldManaged is already false for it. The restricted types are therefore
// any/eq/i31/struct/array/exn and concrete struct/array type indices.
inline bool isRestrictedManagedRefGetterType(bool CanHoldManaged,
                                             const ValType &RT) noexcept {
  return CanHoldManaged && !RT.isExternRefType();
}

} // namespace WasmEdge
