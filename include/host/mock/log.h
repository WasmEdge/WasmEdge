// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include <string_view>

#include "common/spdlog.h"

namespace WasmEdge {
namespace Host {

inline void printPluginMock(std::string_view PluginName) {
  using namespace std::literals;
  spdlog::error("{} plugin not installed. Please install the plugin and "
                "restart WasmEdge."sv,
                PluginName);
}

} // namespace Host
} // namespace WasmEdge
