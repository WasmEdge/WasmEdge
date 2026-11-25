// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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

/// The host module `Mod`, or null if a host function of it could not be
/// added. Logs the first such error.
std::unique_ptr<Runtime::Instance::ModuleInstance>
checkHostModule(std::unique_ptr<Runtime::Instance::ModuleInstance> Mod);

/// Create the module instances for all official plugins, using the real
/// plugin module when the plugin is installed and the mock module otherwise.
/// An official plugin module that cannot be completed is left out.
std::vector<std::unique_ptr<Runtime::Instance::ModuleInstance>>
loadOfficialPluginModules();

/// Check whether the plugin name belongs to an official plugin covered by
/// loadOfficialPluginModules().
bool isOfficialPlugin(std::string_view PName);

} // namespace VM
} // namespace WasmEdge
