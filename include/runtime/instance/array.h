// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
  // Note: These constructors are intentionally NOT marked noexcept. The
  // underlying `Data` vector is sized from a runtime-controlled element count
  // (e.g. the `array.new` / `array.new_default` length operand), so the
  // allocation may throw std::bad_alloc. Keeping them potentially-throwing lets
  // the executor catch the failure and turn it into a Wasm trap instead of
  // aborting the whole host process via std::terminate.
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
