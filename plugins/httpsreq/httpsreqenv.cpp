// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "httpsreqenv.h"
#include "httpsreqmodule.h"   

namespace WasmEdge {
namespace Host {

namespace {

Runtime::Instance::ModuleInstance *create(void) noexcept {
  return new HttpsReqModule;
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "https_req",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 10, 1, 0},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "https_req",
                .Description = "",
                .Create = create,
            },
        },
    .AddOptions = nullptr,
};

} // namespace

Plugin::PluginRegister HttpsReqEnvironment::Register(&Descriptor);

} // namespace Host
} // namespace WasmEdge
