// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/runtime/instance/struct.h - Struct Instance definition ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the struct instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/type.h"
#include "common/span.h"
#include "common/types.h"
#include "gc/allocator.h"
#include "runtime/instance/gc.h"

#include <vector>

namespace WasmEdge::Runtime::Instance {

class StructInstance : public GCInstance {
public:
  StructInstance() = delete;
  StructInstance(GC::Allocator &Allocator, const ModuleInstance *ModInst,
                 uint32_t TypeIdx, std::vector<ValVariant> &&Init) noexcept {
    assuming(Init.size() <=
             (std::numeric_limits<uint32_t>::max() - sizeof(RawData)) /
                 sizeof(ValVariant));
    Data = static_cast<RawData *>(Allocator.allocate(
        [&](void *Pointer) {
          auto Raw = static_cast<RawData *>(Pointer);
          Raw->ModInst = ModInst;
          Raw->TypeIdx = TypeIdx;
          Raw->Length = static_cast<uint32_t>(Init.size());
          std::copy(Init.begin(), Init.end(), Raw->Data);
        },
        static_cast<uint32_t>(sizeof(RawData) +
                              Init.size() * sizeof(ValVariant))));
  }
  StructInstance(RawData *Raw) noexcept : GCInstance(Raw) {}

  /// Get field data in struct instance.
  ValVariant &getField(uint32_t Idx) noexcept { return Data->Data[Idx]; }
  const ValVariant &getField(uint32_t Idx) const noexcept {
    return Data->Data[Idx];
  }
};

} // namespace WasmEdge::Runtime::Instance
