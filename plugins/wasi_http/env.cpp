// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "env.h"
#include "module.h"

namespace WasmEdge {
namespace Host {

WasiHttpEnvironment::WasiHttpEnvironment() noexcept {

  URIs = {"https://www.google.com/", "https://duckduckgo.com/"};
}

namespace {

Runtime::Instance::ModuleInstance *
create(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasiHttpModule();
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasi_http",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 1, 0, 0},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
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
