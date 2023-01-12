// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "httpsreqenv.h"
#include "httpsreqmodule.h"

namespace WasmEdge {
namespace Host {

namespace {

Runtime::Instance::ModuleInstance *
create(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmEdgeHttpsReqModule;
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasmedge_httpsreq",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 10, 1, 0},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasmedge_httpsreq",
                .Description = "",
                .Create = create,
            },
        },
    .AddOptions = nullptr,
};

} // namespace

Plugin::PluginRegister WasmEdgeHttpsReqEnvironment::Register(&Descriptor);

} // namespace Host
} // namespace WasmEdge
