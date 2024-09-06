// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "env.h"
#include "module.h"

namespace WasmEdge {
namespace Host {

namespace {

Runtime::Instance::ComponentInstance *
createTypes(const Plugin::PluginComponent::ComponentDescriptor *) noexcept {
  return new TypesModule();
}

Runtime::Instance::ComponentInstance *
createPreopens(const Plugin::PluginComponent::ComponentDescriptor *) noexcept {
  return new PreopensModule();
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasi_filesystem",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 1, 0, 0},
    .ModuleCount = 0,
    .ModuleDescriptions = {},
    .ComponentCount = 2,
    .ComponentDescriptions =
        (Plugin::PluginComponent::ComponentDescriptor[]){
            {
                .Name = "wasi:filesystem/types@0.2.0",
                .Description = "",
                .Create = createTypes,
            },
            {
                .Name = "wasi:filesystem/preopens@0.2.0",
                .Description = "",
                .Create = createPreopens,
            },
        },
    .AddOptions = nullptr,
};

EXPORT_GET_DESCRIPTOR(Descriptor)

} // namespace
} // namespace Host
} // namespace WasmEdge
