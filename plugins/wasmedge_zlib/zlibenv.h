// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "plugin/plugin.h"

#include <cstdint>
#include <memory>
#include <unordered_map>

#include <zlib.h>

namespace WasmEdge {
namespace Host {

class WasmEdgeZlibEnvironment {
public:
  std::unordered_map<uint32_t, std::unique_ptr<z_stream>> ZStreamMap;

  /// Initial Configurations
  static Plugin::PluginRegister Register;
};

} // namespace Host
} // namespace WasmEdge
