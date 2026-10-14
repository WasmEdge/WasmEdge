// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/array.h - Array Instance definition -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the array instance definition in the GC heap.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"
#include "common/span.h"
#include "common/types.h"
#include "gc/allocator.h"
#include "runtime/instance/gc.h"

#include <memory>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ArrayInstance : public GCInstance {
public:
  ArrayInstance() = delete;
  ArrayInstance(GC::Allocator &Allocator, const ModuleInstance *ModInst,
                uint32_t TypeIdx, uint32_t Size,
                const ValVariant &Init) noexcept {
    assuming(ModInst);
    // A null Data (allocation failure or size overflow) makes arrayNew() trap
    // with GCAllocationFailed.
    Data = allocateRaw(Allocator, ModInst, TypeIdx, Size,
                       [&](ValVariant *Payload) noexcept {
                         std::uninitialized_fill_n(Payload, Size, Init);
                       });
  }
  ArrayInstance(GC::Allocator &Allocator, const ModuleInstance *ModInst,
                uint32_t TypeIdx, std::vector<ValVariant> &&Init) noexcept {
    assuming(ModInst);
    Data = allocateRaw(Allocator, ModInst, TypeIdx, Init.size(),
                       [&](ValVariant *Payload) noexcept {
                         std::uninitialized_copy(Init.begin(), Init.end(),
                                                 Payload);
                       });
  }
  explicit ArrayInstance(RawData *Raw) noexcept : GCInstance(Raw) {}

  /// Get field data in array instance.
  /// Data is null only on allocation failure, which arrayNew() traps before any
  /// instance reaches these accessors (callers null-check first).
  ValVariant &getData(uint32_t Idx) noexcept {
    assuming(Data);
    assuming(Idx < Data->Length);
    return Data->data()[Idx];
  }
  const ValVariant &getData(uint32_t Idx) const noexcept {
    assuming(Data);
    assuming(Idx < Data->Length);
    return Data->data()[Idx];
  }

  /// Get full array as a read-only span, for reads that do not race the
  /// marker. There is no mutable span: element writes must go through
  /// GC::storeCoherent.
  Span<const ValVariant> getArray() const noexcept {
    assuming(Data);
    return {Data->data(), Data->Length};
  }

  /// Get array length.
  uint32_t getLength() const noexcept {
    assuming(Data);
    return Data->Length;
  }
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
