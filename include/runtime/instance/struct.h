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
                 uint32_t TypeIdx, std::vector<ValVariant> &&Init) noexcept;
  StructInstance(RawData *Raw) noexcept : GCInstance(Raw) {}

  /// Get field data in struct instance.
  ValVariant &getField(uint32_t Idx) noexcept { return Data->Data[Idx]; }
  const ValVariant &getField(uint32_t Idx) const noexcept {
    return Data->Data[Idx];
  }
};

} // namespace WasmEdge::Runtime::Instance
