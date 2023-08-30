// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#include "env.h"
#include "module.h"

namespace WasmEdge {
namespace Host {

WasmEdgeWasiClocksEnvironment::WasmEdgeWasiClocksEnvironment() noexcept {}

namespace {

Runtime::Instance::ModuleInstance *
create(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmEdgeWasiClocksModule();
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasmedge_wasi_clocks",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 1, 0, 0},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasmedge_wasi_clocks",
                .Description = "",
                .Create = create,
            },
        },
    .AddOptions = nullptr,
};

} // namespace

Plugin::PluginRegister WasmEdgeWasiClocksEnvironment::Register(&Descriptor);

} // namespace Host
} // namespace WasmEdge
