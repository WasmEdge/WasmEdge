// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/module.h"
#include "common/func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasiCryptoCommonModule::WasiCryptoCommonModule(
    std::shared_ptr<WasiCrypto::Context> C)
    : ModuleInstance("wasi_ephemeral_crypto_common"), Ctx(C) {
  using namespace WasiCrypto;

  addHostFunc("array_output_len",
              std::make_unique<Common::ArrayOutputLen>(*Ctx));
  addHostFunc("array_output_pull",
              std::make_unique<Common::ArrayOutputPull>(*Ctx));
  addHostFunc("options_open", std::make_unique<Common::OptionsOpen>(*Ctx));
  addHostFunc("options_close", std::make_unique<Common::OptionsClose>(*Ctx));
  addHostFunc("options_set", std::make_unique<Common::OptionsSet>(*Ctx));
  addHostFunc("options_set_u64", std::make_unique<Common::OptionsSetU64>(*Ctx));
  addHostFunc("options_set_guest_buffer",
              std::make_unique<Common::OptionsSetGuestBuffer>(*Ctx));
  addHostFunc("secrets_manager_open",
              std::make_unique<Common::SecretsManagerOpen>(*Ctx));
  addHostFunc("secrets_manager_close",
              std::make_unique<Common::SecretsManagerClose>(*Ctx));
  addHostFunc("secrets_manager_invalidate",
              std::make_unique<Common::SecretsManagerInvalidate>(*Ctx));
}

} // namespace Host
} // namespace WasmEdge
