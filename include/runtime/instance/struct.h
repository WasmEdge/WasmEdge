// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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
    // Guard hard (assuming() is a release no-op): on overflow leave Data ==
    // nullptr so structNew() traps (AccessNullStruct) instead of
    // under-allocating. Defense-in-depth; field count is validator-bounded.
    if (Init.size() > (std::numeric_limits<uint32_t>::max() - sizeof(RawData)) /
                          sizeof(ValVariant)) {
      return;
    }
    Data = static_cast<RawData *>(Allocator.allocate(
        [&](void *Pointer) noexcept {
          // Start the RawData lifetime, then construct each payload field in
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
  explicit StructInstance(RawData *Raw) noexcept : GCInstance(Raw) {}

  /// Get field data in struct instance.
  /// Data is null only on allocation failure, which structNew() traps before
  /// any instance reaches these accessors (callers null-check first).
  ValVariant &getField(uint32_t Idx) noexcept {
    assuming(Data);
    assuming(Idx < Data->Length);
    return Data->data()[Idx];
  }
  const ValVariant &getField(uint32_t Idx) const noexcept {
    assuming(Data);
    assuming(Idx < Data->Length);
    return Data->data()[Idx];
  }
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
