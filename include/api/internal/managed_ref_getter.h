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

#include "common/types.h"

namespace WasmEdge {

// A standalone C-API table or global has no owning module and thus no type
// list, so AST::TypeMatcher::refTypeCanHoldGCObject's module-relative
// resolution of a concrete heap-type index is unavailable. Resolve what can be
// resolved from the ref type alone: an abstract managed ref type (any/eq/i31/
// struct/array/extern/exn) is true, abstract funcref/nullfuncref is (provably)
// false, and a concrete type index -- indistinguishable here from a
// function-type index -- conservatively resolves to true (the safe direction:
// it may in fact be a struct/array type).
inline bool resolveCanHoldManagedNoTypeList(const ValType &RT) noexcept {
  return RT.isRefType() && !(RT.isFuncRefType() && RT.isAbsHeapType());
}

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
