// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "env.h"
#include "module.h"

namespace WasmEdge {
namespace Host {

namespace {

Runtime::Instance::ComponentInstance *createEnvironment(
    const Plugin::PluginComponent::ComponentDescriptor *) noexcept {
  return new WasiCliEnvironmentModule();
}

Runtime::Instance::ComponentInstance *
createExit(const Plugin::PluginComponent::ComponentDescriptor *) noexcept {
  return new WasiCliExitModule();
}

Runtime::Instance::ComponentInstance *
createStdin(const Plugin::PluginComponent::ComponentDescriptor *) noexcept {
  return new WasiCliStdinModule();
}

Runtime::Instance::ComponentInstance *
createStdout(const Plugin::PluginComponent::ComponentDescriptor *) noexcept {
  return new WasiCliStdoutModule();
}

Runtime::Instance::ComponentInstance *
createStderr(const Plugin::PluginComponent::ComponentDescriptor *) noexcept {
  return new WasiCliStderrModule();
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasi_cli",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 1, 0, 0},
    .ModuleCount = 0,
    .ModuleDescriptions = {},
    .ComponentCount = 5,
    .ComponentDescriptions =
        (Plugin::PluginComponent::ComponentDescriptor[]){
            {
                .Name = "wasi:cli/environment@0.2.0",
                .Description = "",
                .Create = createEnvironment,
            },
            {
                .Name = "wasi:cli/exit@0.2.0",
                .Description = "",
                .Create = createExit,
            },
            {
                .Name = "wasi:cli/stdin@0.2.0",
                .Description = "",
                .Create = createStdin,
            },
            {
                .Name = "wasi:cli/stdout@0.2.0",
                .Description = "",
                .Create = createStdout,
            },
            {
                .Name = "wasi:cli/stderr@0.2.0",
                .Description = "",
                .Create = createStderr,
            },
        },
    .AddOptions = nullptr,
};

EXPORT_GET_DESCRIPTOR(Descriptor)

} // namespace
} // namespace Host
} // namespace WasmEdge
