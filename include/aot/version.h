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
// instead of mis-reading it. Bump only once per release: every ABI change that
// lands before the next release belongs under the same version.
//
// v3 (shipped in 0.18.0-alpha.1): memory.size, table.size, table.get and
// table.set were inlined and their intrinsics dropped.
//
// v4: the GC proposal appended kWriteBarrier and kGCSafepoint (the cooperative
// safepoint poll) to the intrinsics table and added the ShadowHead and
// GCStopFlag pointers to the ExecCtx struct. Compiled code marks GC capability
// with the "gc.capable" global, and the AOT section of a universal WASM records
// it as a flag byte after the architecture type. A v3 runtime would misread
// every field after that byte, and a v4 runtime would find no marker in a v3
// artifact.
static inline constexpr const uint32_t kBinaryVersion [[maybe_unused]] = 4;

} // namespace AOT
} // namespace WasmEdge
