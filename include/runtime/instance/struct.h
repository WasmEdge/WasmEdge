// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/runtime/instance/struct.h - Struct Instance definition ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the struct instance definition in the GC heap.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"
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

class StructInstance : public GCInstance {
public:
  StructInstance() = delete;
  StructInstance(GC::Allocator &Allocator, const ModuleInstance *ModInst,
                 uint32_t TypeIdx, std::vector<ValVariant> &&Init) noexcept {
    assuming(ModInst);
    // Guard hard (assuming() is a no-op in release): on overflow leave
    // Data == nullptr so structNew() traps (AccessNullStruct) instead of
    // under-allocating. The field count is validator-bounded so this is
    // defense-in-depth, mirroring ArrayInstance.
    if (Init.size() > (std::numeric_limits<uint32_t>::max() - sizeof(RawData)) /
                          sizeof(ValVariant)) {
      return;
    }
    Data = static_cast<RawData *>(Allocator.allocate(
        [&](void *Pointer) noexcept {
          // Start the RawData object's lifetime explicitly, then construct each
          // payload field in place; the raw storage holds no live objects yet.
          auto *Raw = new (Pointer) RawData;
          Raw->ModInst = ModInst;
          Raw->TypeIdx = TypeIdx;
          Raw->Length = static_cast<uint32_t>(Init.size());
          std::uninitialized_copy(Init.begin(), Init.end(), Raw->Data);
        },
        static_cast<uint32_t>(sizeof(RawData) +
                              Init.size() * sizeof(ValVariant))));
  }
  explicit StructInstance(RawData *Raw) noexcept : GCInstance(Raw) {}

  /// Get field data in struct instance.
  /// Data may be null only on allocation failure, which structNew() turns into
  /// a trap before any instance reaches these accessors; callers reach them
  /// only after a null-reference check.
  ValVariant &getField(uint32_t Idx) noexcept {
    assuming(Data);
    assuming(Idx < Data->Length);
    return Data->Data[Idx];
  }
  const ValVariant &getField(uint32_t Idx) const noexcept {
    assuming(Data);
    assuming(Idx < Data->Length);
    return Data->Data[Idx];
  }
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
