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

#include <cstddef>
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
    /// Owning module instance, used to resolve TypeIdx during GC marking. Must
    /// outlive this object; the GC heap does not extend its lifetime.
    const ModuleInstance *ModInst;
    /// Defined type index of this struct/array within ModInst's type section.
    uint32_t TypeIdx;
    /// Number of ValVariant elements in the payload (count, not bytes).
    uint32_t Length;
    /// Flexible payload of Length ValVariant fields/elements, via data().
    ///
    /// Raw ValVariant-aligned bytes rather than `ValVariant Data[]`: GCC < 10
    /// can't synthesize this struct's default ctor for a flexible array member
    /// of class type (it rejects the placement `new (Pointer) RawData` in
    /// Struct/ArrayInstance with "unknown array size in delete"). std::byte has
    /// a trivial default ctor, so no per-element construction is generated;
    /// each ValVariant is placement-constructed into this storage. Layout
    /// matches `ValVariant Data[]` (same offset, size, 16-byte alignment).
    alignas(ValVariant) std::byte Storage[];

    /// Typed view of the payload. The referenced ValVariants are live only once
    /// Struct/ArrayInstance has constructed them (see their allocate
    /// callbacks).
    ValVariant *data() noexcept {
      return reinterpret_cast<ValVariant *>(Storage);
    }
    const ValVariant *data() const noexcept {
      return reinterpret_cast<const ValVariant *>(Storage);
    }
  };
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
#pragma warning(pop)
#endif
  // RawData (via ValVariant's SIMD members) needs 16-byte alignment; the GC
  // allocator over-aligns blocks to alignof(GC::Allocator::Header) == 16 for
  // it. Keep this assert in sync if either alignment changes.
  static_assert(alignof(RawData) <= 16,
                "GC RawData requires stronger alignment than the allocator "
                "guarantees");
  // A ref's payload is read as its leading `const ModuleInstance *` without
  // knowing the concrete type (see RefVariant::getInnerPtr); keep ModInst
  // first.
  static_assert(offsetof(RawData, ModInst) == 0,
                "RawData must begin with the ModInst pointer");

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
