// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include <string_view>

#include "common/spdlog.h"

namespace WasmEdge {
namespace Host {

inline void printPluginMock(std::string_view PluginName) {
  spdlog::error("{} plugin not installed. Please install the plugin and "
                "restart WasmEdge.",
                PluginName);
}

} // namespace Host
} // namespace WasmEdge
