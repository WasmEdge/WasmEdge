// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#pragma once

#include "plugin/plugin.h"

#include <cstdint>
#include <vector>

namespace WasmEdge {
namespace Host {

using Pollable = uint32_t;

class WasiPollEnvironment {
public:
  bool isPollable(Pollable P) noexcept;

private:
  std::unordered_map<Pollable, bool> PollableMap;
};

} // namespace Host
} // namespace WasmEdge
