// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "plugin/plugin.h"

#include <cstdint>
#include <string>

namespace WasmEdge {
namespace Host {

class WasmEdgeHttpsReqEnvironment {
public:
  std::string Rcv;

  /// Initial Configurations
  static Plugin::PluginRegister Register;
};

} // namespace Host
} // namespace WasmEdge
