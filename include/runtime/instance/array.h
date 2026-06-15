// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
                uint32_t TypeIdx, const uint32_t Size,
                const ValVariant &Init) noexcept {
    // Size is a wasm-controlled operand and assuming() is a no-op in release
    // builds, so guard hard: on overflow leave Data == nullptr so arrayNew()
    // traps (AccessNullArray) rather than truncating the 64-bit byte size into
    // an undersized uint32_t allocation that std::fill then overflows.
    if (Size > (std::numeric_limits<uint32_t>::max() - sizeof(RawData)) /
                   sizeof(ValVariant)) {
      return;
    }
    Data = static_cast<RawData *>(Allocator.allocate(
        [&](void *Pointer) noexcept {
          // Start the RawData object's lifetime explicitly, then construct each
          // payload element in place; the raw storage holds no live objects
          // yet.
          auto *Raw = new (Pointer) RawData;
          Raw->ModInst = ModInst;
          Raw->TypeIdx = TypeIdx;
          Raw->Length = Size;
          std::uninitialized_fill_n(Raw->Data, Size, Init);
        },
        static_cast<uint32_t>(sizeof(RawData) + Size * sizeof(ValVariant))));
  }
  ArrayInstance(GC::Allocator &Allocator, const ModuleInstance *ModInst,
                uint32_t TypeIdx, std::vector<ValVariant> &&Init) noexcept {
    if (Init.size() > (std::numeric_limits<uint32_t>::max() - sizeof(RawData)) /
                          sizeof(ValVariant)) {
      return;
    }
    Data = static_cast<RawData *>(Allocator.allocate(
        [&](void *Pointer) noexcept {
          // Start the RawData object's lifetime explicitly, then construct each
          // payload element in place; the raw storage holds no live objects
          // yet.
          auto *Raw = new (Pointer) RawData;
          Raw->ModInst = ModInst;
          Raw->TypeIdx = TypeIdx;
          Raw->Length = static_cast<uint32_t>(Init.size());
          std::uninitialized_copy(Init.begin(), Init.end(), Raw->Data);
        },
        static_cast<uint32_t>(sizeof(RawData) +
                              Init.size() * sizeof(ValVariant))));
  }
  explicit ArrayInstance(RawData *Raw) noexcept : GCInstance(Raw) {}

  /// Get field data in array instance.
  /// Data may be null only on allocation failure, which arrayNew() turns into a
  /// trap before any instance reaches these accessors; callers reach them only
  /// after a null-reference check.
  ValVariant &getData(uint32_t Idx) noexcept {
    assuming(Data);
    assuming(Idx < Data->Length);
    return Data->Data[Idx];
  }
  const ValVariant &getData(uint32_t Idx) const noexcept {
    assuming(Data);
    assuming(Idx < Data->Length);
    return Data->Data[Idx];
  }

  /// Get full array.
  Span<ValVariant> getArray() noexcept {
    assuming(Data);
    return {Data->Data, Data->Length};
  }
  Span<const ValVariant> getArray() const noexcept {
    assuming(Data);
    return {Data->Data, Data->Length};
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
