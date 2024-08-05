// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

// BUILTIN-PLUGIN: Temporary move the wasi-logging plugin sources here until
// the new plugin architecture ready in 0.15.0.

#pragma once

#include "env.h"

#include "plugin/plugin.h"
#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasiLoggingModule : public Runtime::Instance::ModuleInstance {
public:
  WasiLoggingModule();

  WASILogging::LogEnv &getEnv() { return Env; }

  static Plugin::Plugin::PluginDescriptor PluginDescriptor;
  static Plugin::PluginModule::ModuleDescriptor ModuleDescriptor[];

private:
  WASILogging::LogEnv Env;
};

} // namespace Host
} // namespace WasmEdge
