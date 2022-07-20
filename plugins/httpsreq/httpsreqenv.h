// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "plugin/plugin.h"
#include "po/argument_parser.h"
#include "po/list.h"
#include "po/option.h"
#include <cstdint>
#include <string>
#include <vector>

namespace WasmEdge {
namespace Host {

class HttpsReqEnvironment {
public:
  /// Define your environment variables and data structures

  /// Const
  /// Variables
  std::string Host;
  uint32_t Port;
  std::string BodyStr;

  /// Initial Configurations
  static Plugin::PluginRegister Register;
};

} // namespace Host
} // namespace WasmEdge