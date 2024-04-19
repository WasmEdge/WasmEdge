// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "env.h"
#include "module.h"

namespace WasmEdge {
namespace Host {

WasiHttpEnvironment::WasiHttpEnvironment() noexcept {}

namespace {

Runtime::Instance::ComponentInstance *
create(const Plugin::PluginComponent::ComponentDescriptor *) noexcept {
  return new WasiHttpModule();
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasi_http",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 1, 0, 0},
    .ModuleCount = 0,
    .ModuleDescriptions = {},
    .ComponentCount = 1,
    .ComponentDescriptions =
        (Plugin::PluginComponent::ComponentDescriptor[]){
            {
                .Name = "wasi:http/test",
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
