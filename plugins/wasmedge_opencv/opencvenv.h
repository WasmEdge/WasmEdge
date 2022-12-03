// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "plugin/plugin.h"

#include <cstdint>
#include <string>

namespace WasmEdge {
namespace Host {

class WasmEdgeOpenCvEnvironment {
public:
  std::string Rcv;

  /// Initial Configurations
  static Plugin::PluginRegister Register;

  uint32_t ExitCode = 0;
};

} // namespace Host
} // namespace WasmEdge
