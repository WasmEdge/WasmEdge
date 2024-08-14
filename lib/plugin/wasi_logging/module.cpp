// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

// BUILTIN-PLUGIN: Temporary move the wasi-logging plugin sources here until
// the new plugin architecture ready in 0.15.0.

#include "plugin/wasi_logging/module.h"
#include "plugin/plugin.h"
#include "plugin/wasi_logging/func.h"

#include <string_view>

namespace WasmEdge {
namespace Host {

namespace {
Runtime::Instance::ModuleInstance *
create(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasiLoggingModule;
}
} // namespace

using namespace std::literals;

const std::string WASILogging::LogEnv::DefFormat =
    "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v";
std::mutex WASILogging::LogEnv::Mutex;
std::unordered_set<uint64_t> WASILogging::LogEnv::RegisteredID;

WasiLoggingModule::WasiLoggingModule()
    : ModuleInstance("wasi:logging/logging"sv) {
  addHostFunc("log"sv, std::make_unique<WASILogging::Log>(Env));
}

Plugin::PluginModule::ModuleDescriptor WasiLoggingModule::ModuleDescriptor[]{
    {
        /* Name */ "wasi:logging/logging",
        /* Description */ "",
        /* Create */ create,
    },
};

Plugin::Plugin::PluginDescriptor WasiLoggingModule::PluginDescriptor{
    /* Name */ "wasi_logging",
    /* Description */ "",
    /* APIVersion */ Plugin::Plugin::CurrentAPIVersion,
    /* Version */ {0, 1, 0, 0},
    /* ModuleCount */ 1,
    /* ModuleDescriptions */ ModuleDescriptor,
    /* ComponentCount */ 0,
    /* ComponentDescriptions */ nullptr,
    /* AddOptions */ nullptr,
};

} // namespace Host
} // namespace WasmEdge
