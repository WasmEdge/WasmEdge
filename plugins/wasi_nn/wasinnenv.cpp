// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "wasinnenv.h"
#include "wasinnmodule.h"

namespace WasmEdge {
namespace Host {

namespace {

void addOptions(PO::ArgumentParser &) noexcept {}

Runtime::Instance::ModuleInstance *create(void) noexcept {
  return new WasiNNModule;
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasi_nn",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 10, 0, 0},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasi_nn",
                .Description = "",
                .Create = create,
            },
        },
    .AddOptions = addOptions,
};

} // namespace

Plugin::PluginRegister WASINN::WasiNNEnvironment::Register(&Descriptor);

} // namespace Host
} // namespace WasmEdge
