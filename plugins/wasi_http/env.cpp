// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "env.h"
#include "interface.h"
#include "module.h"

namespace WasmEdge {
namespace Host {

WasiHttpEnvironment::WasiHttpEnvironment() noexcept {}

namespace {

Runtime::Instance::ComponentInstance *
create(const Plugin::PluginComponent::ComponentDescriptor *) noexcept {
  return new WasiHttpModule();
}

Runtime::Instance::ComponentInstance *
createTypes(const Plugin::PluginComponent::ComponentDescriptor *) noexcept {
  return new WasiHttp_Types();
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasi_http",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 1, 0, 0},
    .ModuleCount = 0,
    .ModuleDescriptions = {},
    .ComponentCount = 2,
    .ComponentDescriptions =
        (Plugin::PluginComponent::ComponentDescriptor[]){
            {
                .Name = "wasi:http/test",
                .Description = "",
                .Create = create,
            },
            {
                .Name = "wasi:http/types@0.2.0",
                .Description = "",
                .Create = createTypes,
            },
        },
    .AddOptions = nullptr,
};

EXPORT_GET_DESCRIPTOR(Descriptor)

} // namespace
} // namespace Host
} // namespace WasmEdge
