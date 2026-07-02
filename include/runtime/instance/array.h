// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/array.h - Array Instance definition -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the array instance definition in store manager.
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

class ArrayInstance : public CompositeBase {
public:
  ArrayInstance() = delete;
  // Not noexcept: Data is sized from a runtime element count and may throw
  // (bad_alloc/length_error), which the executor catches and turns into a trap.
  ArrayInstance(const ModuleInstance *Mod, const uint32_t Idx,
                const uint32_t Size, const ValVariant &Init)
      : CompositeBase(Mod, Idx), Data(Size, Init) {
    assuming(ModInst);
  }
  ArrayInstance(const ModuleInstance *Mod, const uint32_t Idx,
                std::vector<ValVariant> &&Init)
      : CompositeBase(Mod, Idx), Data(std::move(Init)) {
    assuming(ModInst);
  }

  /// Get field data in array instance.
  ValVariant &getData(uint32_t Idx) noexcept { return Data[Idx]; }
  const ValVariant &getData(uint32_t Idx) const noexcept { return Data[Idx]; }

  /// Get full array.
  Span<ValVariant> getArray() noexcept { return Data; }
  Span<const ValVariant> getArray() const noexcept { return Data; }

  /// Get array length.
  uint32_t getLength() const noexcept {
    return static_cast<uint32_t>(Data.size());
  }

private:
  /// \name Data of array instance.
  /// @{
  std::vector<ValVariant> Data;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
