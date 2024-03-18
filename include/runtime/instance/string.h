// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ast/type.h"
#include "common/span.h"
#include "common/types.h"
#include "runtime/instance/composite.h"

#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class StringInstance : public CompositeBase {
public:
  StringInstance() = delete;
  StringInstance(const ModuleInstance *Mod, const uint32_t Idx,
                 const uint32_t Size) noexcept
      : CompositeBase(Mod, Idx), Data(nullptr, Size) {
    assuming(ModInst);
  }
  StringInstance(const ModuleInstance *Mod, const uint32_t Idx,
                 const uint32_t Size, const char &Init) noexcept
      : CompositeBase(Mod, Idx), Data(Init, Size) {
    assuming(ModInst);
  }
  StringInstance(const ModuleInstance *Mod, const uint32_t Idx,
                 std::string &&Init) noexcept
      : CompositeBase(Mod, Idx), Data(std::move(Init)) {
    assuming(ModInst);
  }

  std::string &getString() noexcept { return Data; }
  std::string_view getString() const noexcept { return Data; }

  uint32_t getLength() const noexcept {
    return static_cast<uint32_t>(Data.size());
  }

  /// Get boundary index.
  uint32_t getBoundIdx() const noexcept {
    return std::max(static_cast<uint32_t>(Data.size()), UINT32_C(1)) -
           UINT32_C(1);
  }

private:
  /// \name Data of array instance.
  /// @{
  std::string Data;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
