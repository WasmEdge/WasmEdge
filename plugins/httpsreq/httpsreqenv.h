// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "plugin/plugin.h"

#include <cstdint>
#include <string>

namespace WasmEdge {
namespace Host {

class HttpsReqEnvironment {
public:
  std::string Host;
  uint32_t Port;
  std::string BodyStr;
  std::string Rcv;

  /// Initial Configurations
  static Plugin::PluginRegister Register;
};

} // namespace Host
} // namespace WasmEdge
