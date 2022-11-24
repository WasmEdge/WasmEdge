// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "opencvenv.h"
#include "coremodule.h"

namespace WasmEdge {
namespace Host {

namespace {

Runtime::Instance::ModuleInstance *
create(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmEdgeOpenCVCore;
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasmedge_opencv",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 10, 1, 0},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasmedge_opencv",
                .Description = "",
                .Create = create,
            },
        },
    .AddOptions = nullptr,
};

} // namespace

Plugin::PluginRegister WasmEdgeOpenCVEnvironment::Register(&Descriptor);

} // namespace Host
} // namespace WasmEdge
