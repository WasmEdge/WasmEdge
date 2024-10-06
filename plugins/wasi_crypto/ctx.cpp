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

Runtime::Instance::ModuleInstance *createAsymmetricCommon(
    const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasiCryptoAsymmetricCommonModule(
      WasiCrypto::Context::getInstance());
}
Runtime::Instance::ModuleInstance *
createCommon(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasiCryptoCommonModule(WasiCrypto::Context::getInstance());
}
Runtime::Instance::ModuleInstance *
createKx(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasiCryptoKxModule(WasiCrypto::Context::getInstance());
}
Runtime::Instance::ModuleInstance *
createSignatures(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasiCryptoSignaturesModule(WasiCrypto::Context::getInstance());
}
Runtime::Instance::ModuleInstance *
createSymmetric(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasiCryptoSymmetricModule(WasiCrypto::Context::getInstance());
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
                .Create = createAsymmetricCommon,
            },
            {
                .Name = "wasi_crypto_common",
                .Description = "",
                .Create = createCommon,
            },
            {
                .Name = "wasi_crypto_kx",
                .Description = "",
                .Create = createKx,
            },
            {
                .Name = "wasi_crypto_signatures",
                .Description = "",
                .Create = createSignatures,
            },
            {
                .Name = "wasi_crypto_symmetric",
                .Description = "",
                .Create = createSymmetric,
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
