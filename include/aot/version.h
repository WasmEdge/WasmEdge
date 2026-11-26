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

#include "runtime/refheap.h"

#include <cstdint>

namespace WasmEdge {
namespace AOT {

static inline constexpr const uint32_t kBinaryVersion [[maybe_unused]] = 4;

/// Flag in the version word of a binary that decodes chunk mode offsets.
static inline constexpr const uint32_t kChunkModeVersion [[maybe_unused]] =
    UINT32_C(1) << 31;

/// Version word of a binary whose code decodes offsets of a heap in `Mode`.
inline uint32_t getBinaryVersion(Runtime::HeapMode Mode) noexcept {
  return Mode == Runtime::HeapMode::Chunk ? kBinaryVersion | kChunkModeVersion
                                          : kBinaryVersion;
}

/// Version word of a binary for the mode of the global heap.
inline uint32_t getBinaryVersion() noexcept {
  return getBinaryVersion(Runtime::RefHeap::getGlobalMode());
}

} // namespace AOT
} // namespace WasmEdge
