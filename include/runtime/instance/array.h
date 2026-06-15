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

#include <limits>
#include <memory>
#include <new>
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
    // Size is wasm-controlled and assuming() is a release no-op, so guard hard:
    // on overflow leave Data == nullptr so arrayNew() traps
    // (GCAllocationFailed) rather than truncate the byte size into an
    // undersized allocation that std::fill then overflows.
    if (Size > (std::numeric_limits<uint32_t>::max() - sizeof(RawData)) /
                   sizeof(ValVariant)) {
      return;
    }
    Data = static_cast<RawData *>(Allocator.allocate(
        [&](void *Pointer) noexcept {
          // Start the RawData lifetime, then construct each payload element in
          // place (raw storage holds no live objects yet).
          auto *Raw = new (Pointer) RawData;
          Raw->ModInst = ModInst;
          Raw->TypeIdx = TypeIdx;
          Raw->Length = Size;
          std::uninitialized_fill_n(Raw->data(), Size, Init);
        },
        static_cast<uint32_t>(sizeof(RawData) + Size * sizeof(ValVariant))));
  }
  ArrayInstance(GC::Allocator &Allocator, const ModuleInstance *ModInst,
                uint32_t TypeIdx, std::vector<ValVariant> &&Init) noexcept {
    assuming(ModInst);
    if (Init.size() > (std::numeric_limits<uint32_t>::max() - sizeof(RawData)) /
                          sizeof(ValVariant)) {
      return;
    }
    Data = static_cast<RawData *>(Allocator.allocate(
        [&](void *Pointer) noexcept {
          // Start the RawData lifetime, then construct each payload element in
          // place (raw storage holds no live objects yet).
          auto *Raw = new (Pointer) RawData;
          Raw->ModInst = ModInst;
          Raw->TypeIdx = TypeIdx;
          Raw->Length = static_cast<uint32_t>(Init.size());
          std::uninitialized_copy(Init.begin(), Init.end(), Raw->data());
        },
        static_cast<uint32_t>(sizeof(RawData) +
                              Init.size() * sizeof(ValVariant))));
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

  /// Get full array.
  Span<ValVariant> getArray() noexcept {
    assuming(Data);
    return {Data->data(), Data->Length};
  }
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
