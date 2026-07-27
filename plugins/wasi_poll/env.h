// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors
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
  void dropPollable(Pollable P);

private:
  std::unordered_map<Pollable, bool> PollableMap;
};

} // namespace Host
} // namespace WasmEdge
