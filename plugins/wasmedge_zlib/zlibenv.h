// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "plugin/plugin.h"

#include <cstdint>
#include <string>

struct Util {
  std::unordered_map<uint32_t, z_stream *> stream_map;
};

namespace WasmEdge {
namespace Host {

class WasmEdgeZlibEnvironment {
public:
  Util Util;

  /// Initial Configurations
  static Plugin::PluginRegister Register;
};

} // namespace Host
} // namespace WasmEdge
