// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/runtime/instance/gc.h - GC Instance definition -----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the GC data instance definition in the GC heap.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/types.h"

#include <cstdint>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ModuleInstance;

class GCInstance {
public:
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
#pragma warning(push)
#pragma warning(disable : 4200)
#endif
  struct RawData {
    /// Owning module instance, used to resolve TypeIdx during GC marking. The
    /// referenced ModuleInstance must outlive this object; the GC heap does not
    /// extend its lifetime.
    const ModuleInstance *ModInst;
    /// Defined type index of this struct/array within ModInst's type section.
    uint32_t TypeIdx;
    /// Number of ValVariant elements stored in Data (element count, not bytes).
    uint32_t Length;
    /// Flexible payload of Length ValVariant fields/elements.
    ValVariant Data[];
  };
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
#pragma warning(pop)
#endif
  // RawData (via ValVariant's SIMD members) needs 16-byte alignment; the GC
  // allocator over-aligns its blocks to alignof(GC::Allocator::Header) == 16 to
  // satisfy this. Keep this assert in sync if either alignment changes.
  static_assert(alignof(RawData) <= 16,
                "GC RawData requires stronger alignment than the allocator "
                "guarantees");

  explicit GCInstance(RawData *Raw) noexcept : Data(Raw) {}

  RawData *getRaw() noexcept { return Data; }
  const RawData *getRaw() const noexcept { return Data; }

protected:
  GCInstance() noexcept = default;

  /// \name Data of GC instance.
  /// @{
  RawData *Data = nullptr;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
