// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "env.h"
#include "module.h"

namespace WasmEdge {
namespace Host {

namespace {

Runtime::Instance::ComponentInstance *
createError(const Plugin::PluginComponent::ComponentDescriptor *) noexcept {
  return new WasiIOErrorModule();
}

Runtime::Instance::ComponentInstance *
createStreams(const Plugin::PluginComponent::ComponentDescriptor *) noexcept {
  return new WasiIOStreamsModule();
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasi_io",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 1, 0, 0},
    .ModuleCount = 0,
    .ModuleDescriptions = {},
    .ComponentCount = 2,
    .ComponentDescriptions =
        (Plugin::PluginComponent::ComponentDescriptor[]){
            {
                .Name = "wasi:io/error@0.2.0",
                .Description = "",
                .Create = createError,
            },
            {
                .Name = "wasi:io/streams@0.2.0",
                .Description = "",
                .Create = createStreams,
            },
        },
    .AddOptions = nullptr,
};

EXPORT_GET_DESCRIPTOR(Descriptor)

} // namespace
} // namespace Host
} // namespace WasmEdge
