// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/aot/version.h - version definition -----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the binary version signature of WasmEdge.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <cstdint>

namespace WasmEdge {
namespace AOT {

// Bump on any AOT ABI change so an old runtime rejects a newer artifact
// instead of mis-reading it, and bump only once per release: v3 has not
// shipped, so every ABI change landing before it does belongs under v3 rather
// than minting a version no released runtime could ever encounter.
//
// v3 so far covers: memory.size, table.size, table.get and table.set were
// inlined and their intrinsics dropped; the GC proposal appended
// kWriteBarrier; and the GC multi-mutator codegen added the ShadowHead and
// GCStopFlag pointers to the ExecCtx struct plus kGCSafepoint (the cooperative
// safepoint poll) to the intrinsics table. Any one of these alone would leave a
// v2 runtime indexing the Intrinsics table incorrectly.
static inline constexpr const uint32_t kBinaryVersion [[maybe_unused]] = 3;

} // namespace AOT
} // namespace WasmEdge
