// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "symmetric/module.h"
#include "symmetric/func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasiCryptoSymmetricModule::WasiCryptoSymmetricModule(
    std::shared_ptr<WasiCrypto::Context> C)
    : ModuleInstance("wasi_ephemeral_crypto_symmetric"), Ctx(C) {
  using namespace WasiCrypto;

  addHostFunc("symmetric_key_generate",
              std::make_unique<Symmetric::KeyGenerate>(*Ctx));
  addHostFunc("symmetric_key_import",
              std::make_unique<Symmetric::KeyImport>(*Ctx));
  addHostFunc("symmetric_key_export",
              std::make_unique<Symmetric::KeyExport>(*Ctx));
  addHostFunc("symmetric_key_close",
              std::make_unique<Symmetric::KeyClose>(*Ctx));
  addHostFunc("symmetric_key_generate_managed",
              std::make_unique<Symmetric::KeyGenerateManaged>(*Ctx));
  addHostFunc("symmetric_key_store_managed",
              std::make_unique<Symmetric::KeyStoreManaged>(*Ctx));
  addHostFunc("symmetric_key_replace_managed",
              std::make_unique<Symmetric::KeyReplaceManaged>(*Ctx));
  addHostFunc("symmetric_key_id", std::make_unique<Symmetric::KeyId>(*Ctx));
  addHostFunc("symmetric_key_from_id",
              std::make_unique<Symmetric::KeyFromId>(*Ctx));
  addHostFunc("symmetric_state_open",
              std::make_unique<Symmetric::StateOpen>(*Ctx));
  addHostFunc("symmetric_state_clone",
              std::make_unique<Symmetric::StateClone>(*Ctx));
  addHostFunc("symmetric_state_options_get",
              std::make_unique<Symmetric::StateOptionsGet>(*Ctx));
  addHostFunc("symmetric_state_options_get_u64",
              std::make_unique<Symmetric::StateOptionsGetU64>(*Ctx));
  addHostFunc("symmetric_state_close",
              std::make_unique<Symmetric::StateClose>(*Ctx));
  addHostFunc("symmetric_state_absorb",
              std::make_unique<Symmetric::StateAbsorb>(*Ctx));
  addHostFunc("symmetric_state_squeeze",
              std::make_unique<Symmetric::StateSqueeze>(*Ctx));
  addHostFunc("symmetric_state_squeeze_tag",
              std::make_unique<Symmetric::StateSqueezeTag>(*Ctx));
  addHostFunc("symmetric_state_squeeze_key",
              std::make_unique<Symmetric::StateSqueezeKey>(*Ctx));
  addHostFunc("symmetric_state_max_tag_len",
              std::make_unique<Symmetric::StateMaxTagLen>(*Ctx));
  addHostFunc("symmetric_state_encrypt",
              std::make_unique<Symmetric::StateEncrypt>(*Ctx));
  addHostFunc("symmetric_state_encrypt_detached",
              std::make_unique<Symmetric::StateEncryptDetached>(*Ctx));
  addHostFunc("symmetric_state_decrypt",
              std::make_unique<Symmetric::StateDecrypt>(*Ctx));
  addHostFunc("symmetric_state_decrypt_detached",
              std::make_unique<Symmetric::StateDecryptDetached>(*Ctx));
  addHostFunc("symmetric_state_ratchet",
              std::make_unique<Symmetric::StateRatchet>(*Ctx));
  addHostFunc("symmetric_tag_len", std::make_unique<Symmetric::TagLen>(*Ctx));
  addHostFunc("symmetric_tag_pull", std::make_unique<Symmetric::TagPull>(*Ctx));
  addHostFunc("symmetric_tag_verify",
              std::make_unique<Symmetric::TagVerify>(*Ctx));
  addHostFunc("symmetric_tag_close",
              std::make_unique<Symmetric::TagClose>(*Ctx));
}

} // namespace Host
} // namespace WasmEdge
