// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "kx/module.h"
#include "kx/func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasiCryptoKxModule::WasiCryptoKxModule(std::shared_ptr<WasiCrypto::Context> C)
    : ModuleInstance("wasi_ephemeral_crypto_kx"), Ctx(C) {
  using namespace WasiCrypto;

  addHostFunc("kx_dh", std::make_unique<Kx::Dh>(*Ctx));
  addHostFunc("kx_encapsulate", std::make_unique<Kx::Encapsulate>(*Ctx));
  addHostFunc("kx_decapsulate", std::make_unique<Kx::Decapsulate>(*Ctx));
}

} // namespace Host
} // namespace WasmEdge
