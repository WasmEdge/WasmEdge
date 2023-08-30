// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#pragma once

#include "plugin/plugin.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeWasiClocksEnvironment {
public:
  WasmEdgeWasiClocksEnvironment() noexcept;

  static Plugin::PluginRegister Register;
};

} // namespace Host
} // namespace WasmEdge
