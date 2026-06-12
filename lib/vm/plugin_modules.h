// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

//===-- wasmedge/lib/vm/plugin_modules.h - Official plugin inventory ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file declares the inventory of official plugin modules used by the
/// VM: real plugin modules when installed, mock fallbacks otherwise.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "runtime/instance/module.h"

#include <memory>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace VM {

/// Create the module instances for all official plugins, using the real
/// plugin module when the plugin is installed and the mock module otherwise.
std::vector<std::unique_ptr<Runtime::Instance::ModuleInstance>>
loadOfficialPluginModules();

/// Check whether the plugin name belongs to an official plugin covered by
/// loadOfficialPluginModules().
bool isOfficialPlugin(std::string_view PName);

} // namespace VM
} // namespace WasmEdge
