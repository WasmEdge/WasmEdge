// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ctx.h"
#include "asymmetric_common/module.h"
#include "common/module.h"
#include "kx/module.h"
#include "signatures/module.h"
#include "symmetric/module.h"

namespace WasmEdge {
namespace Host {

namespace {

template <typename T>
Runtime::Instance::ModuleInstance *
createModule(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new T(WasiCrypto::Context::getInstance());
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasi_crypto",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 10, 1, 0},
    .ModuleCount = 5,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasi_crypto_asymmetric_common",
                .Description = "",
                .Create = createModule<WasiCryptoAsymmetricCommonModule>,
            },
            {
                .Name = "wasi_crypto_common",
                .Description = "",
                .Create = createModule<WasiCryptoCommonModule>,
            },
            {
                .Name = "wasi_crypto_kx",
                .Description = "",
                .Create = createModule<WasiCryptoKxModule>,
            },
            {
                .Name = "wasi_crypto_signatures",
                .Description = "",
                .Create = createModule<WasiCryptoSignaturesModule>,
            },
            {
                .Name = "wasi_crypto_symmetric",
                .Description = "",
                .Create = createModule<WasiCryptoSymmetricModule>,
            },
        },
    .AddOptions = nullptr,
};

EXPORT_GET_DESCRIPTOR(Descriptor)

} // namespace

std::shared_mutex WasiCrypto::Context::Mutex;
std::weak_ptr<WasiCrypto::Context> WasiCrypto::Context::Instance;

} // namespace Host
} // namespace WasmEdge
