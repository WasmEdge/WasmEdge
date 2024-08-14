// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "opencvmini_env.h"
#include "opencvmini_module.h"

namespace WasmEdge {
namespace Host {

WasmEdgeOpenCVMiniEnvironment::WasmEdgeOpenCVMiniEnvironment() noexcept {}

namespace {

Runtime::Instance::ModuleInstance *
create(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmEdgeOpenCVMiniModule();
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasmedge_opencvmini",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 1, 0, 0},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasmedge_opencvmini",
                .Description = "",
                .Create = create,
            },
        },
    .AddOptions = nullptr,
};

EXPORT_GET_DESCRIPTOR(Descriptor)

} // namespace
} // namespace Host
} // namespace WasmEdge
