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
#include "runtime/instance/composite.h"

#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class StructInstance : public CompositeBase {
public:
  StructInstance() = delete;
  StructInstance(const ModuleInstance *Mod, const uint32_t Idx,
                 std::vector<ValVariant> &&Init) noexcept
      : CompositeBase(Mod, Idx), Data(std::move(Init)) {
    assuming(ModInst);
  }

  /// Get field data in struct instance.
  ValVariant &getField(uint32_t Idx) noexcept { return Data[Idx]; }
  const ValVariant &getField(uint32_t Idx) const noexcept { return Data[Idx]; }

private:
  /// \name Data of struct instance.
  /// @{
  std::vector<ValVariant> Data;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
