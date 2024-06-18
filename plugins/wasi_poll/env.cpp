// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "env.h"
#include "module.h"

namespace WasmEdge {
namespace Host {

WasiPollEnvironment::WasiPollEnvironment() noexcept {}

namespace {

Runtime::Instance::ComponentInstance *
create(const Plugin::PluginComponent::ComponentDescriptor *) noexcept {
  return new WasiPollModule();
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasi_poll",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 1, 0, 0},
    .ModuleCount = 0,
    .ModuleDescriptions = {},
    .ComponentCount = 1,
    .ComponentDescriptions =
        (Plugin::PluginComponent::ComponentDescriptor[]){
            {
                .Name = "wasi:poll/poll",
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
