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

#include <memory>
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
    // A null Data (allocation failure or size overflow) makes structNew() trap
    // with GCAllocationFailed. The field count is validator-bounded.
    Data = allocateRaw(Allocator, ModInst, TypeIdx, Init.size(),
                       [&](ValVariant *Payload) noexcept {
                         std::uninitialized_copy(Init.begin(), Init.end(),
                                                 Payload);
                       });
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
