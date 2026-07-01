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
// instead of mis-reading it. v3 added the kWriteBarrier intrinsic; a v2
// runtime's shorter Intrinsics table would be indexed out of bounds.
static inline constexpr const uint32_t kBinaryVersion [[maybe_unused]] = 3;

} // namespace AOT
} // namespace WasmEdge
