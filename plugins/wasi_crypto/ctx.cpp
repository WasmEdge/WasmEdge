// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "ctx.h"
#include "module.h"

namespace WasmEdge {
namespace Host {

namespace {

Runtime::Instance::ModuleInstance *create(void) noexcept {
  return new WasiCryptoModule;
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasi_crypto",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 10, 1, 0},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasi_crypto",
                .Description = "",
                .Create = create,
            },
        },
    .AddOptions = nullptr,
};

} // namespace

Plugin::PluginRegister WasiCrypto::Context::Register(&Descriptor);

} // namespace Host
} // namespace WasmEdge
