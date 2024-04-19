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

// String instance is assumed to be used in component model
//
// The data will be stored in an assigned memory instance, which is dynamic, and
// hence we can not give any fixed memory instance here.
class StringInstance {
public:
  StringInstance() = delete;
  // in this case, we assume no module instance there, this is because
  StringInstance(std::string_view Init) noexcept
      : Data(std::string{Init.begin(), Init.end()}) {}
  StringInstance(std::string &&Init) noexcept : Data(std::move(Init)) {}

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
