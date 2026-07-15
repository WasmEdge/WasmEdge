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
// instead of mis-reading it. v3 covers two changes: memory.size, table.size,
// table.get and table.set were inlined and their intrinsics dropped, and the
// GC proposal appended kWriteBarrier. Either alone would leave a v2 runtime
// indexing the Intrinsics table incorrectly.
static inline constexpr const uint32_t kBinaryVersion [[maybe_unused]] = 3;

} // namespace AOT
} // namespace WasmEdge
