// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/module.h"
#include "host/wasi_crypto/asymmetric_common/func.h"
#include "host/wasi_crypto/common/func.h"
#include "host/wasi_crypto/kx/func.h"
#include "host/wasi_crypto/signatures/func.h"
#include "host/wasi_crypto/symmetric/func.h"

namespace WasmEdge {
namespace Host {

WasiCryptoModule::WasiCryptoModule() : ModuleInstance("wasi_ephemeral_crypto") {
  using namespace WasiCrypto;
  // common
  addHostFunc("array_output_len",
              std::make_unique<Common::ArrayOutputLen>(Ctx));
  addHostFunc("array_output_pull",
              std::make_unique<Common::ArrayOutputPull>(Ctx));
  addHostFunc("options_open", std::make_unique<Common::OptionsOpen>(Ctx));
  addHostFunc("options_close", std::make_unique<Common::OptionsClose>(Ctx));
  addHostFunc("options_set", std::make_unique<Common::OptionsSet>(Ctx));
  addHostFunc("options_set_u64", std::make_unique<Common::OptionsSetU64>(Ctx));
  addHostFunc("options_set_guest_buffer",
              std::make_unique<Common::OptionsSetGuestBuffer>(Ctx));
  addHostFunc("secrets_manager_open",
              std::make_unique<Common::SecretsManagerOpen>(Ctx));
  addHostFunc("secrets_manager_close",
              std::make_unique<Common::SecretsManagerClose>(Ctx));
  addHostFunc("secrets_manager_invalidate",
              std::make_unique<Common::SecretsManagerInvalidate>(Ctx));

  // symmetric
  addHostFunc("symmetric_key_generate",
              std::make_unique<Symmetric::KeyGenerate>(Ctx));
  addHostFunc("symmetric_key_import",
              std::make_unique<Symmetric::KeyImport>(Ctx));
  addHostFunc("symmetric_key_export",
              std::make_unique<Symmetric::KeyExport>(Ctx));
  addHostFunc("symmetric_key_close",
              std::make_unique<Symmetric::KeyClose>(Ctx));
  addHostFunc("symmetric_key_generate_managed",
              std::make_unique<Symmetric::KeyGenerateManaged>(Ctx));
  addHostFunc("symmetric_key_store_managed",
              std::make_unique<Symmetric::KeyStoreManaged>(Ctx));
  addHostFunc("symmetric_key_replace_managed",
              std::make_unique<Symmetric::KeyReplaceManaged>(Ctx));
  addHostFunc("symmetric_key_id", std::make_unique<Symmetric::KeyId>(Ctx));
  addHostFunc("symmetric_key_from_id",
              std::make_unique<Symmetric::KeyFromId>(Ctx));
  addHostFunc("symmetric_state_open",
              std::make_unique<Symmetric::StateOpen>(Ctx));
  addHostFunc("symmetric_state_clone",
              std::make_unique<Symmetric::StateClone>(Ctx));
  addHostFunc("symmetric_state_options_get",
              std::make_unique<Symmetric::StateOptionsGet>(Ctx));
  addHostFunc("symmetric_state_options_get_u64",
              std::make_unique<Symmetric::StateOptionsGetU64>(Ctx));
  addHostFunc("symmetric_state_close",
              std::make_unique<Symmetric::StateClose>(Ctx));
  addHostFunc("symmetric_state_absorb",
              std::make_unique<Symmetric::StateAbsorb>(Ctx));
  addHostFunc("symmetric_state_squeeze",
              std::make_unique<Symmetric::StateSqueeze>(Ctx));
  addHostFunc("symmetric_state_squeeze_tag",
              std::make_unique<Symmetric::StateSqueezeTag>(Ctx));
  addHostFunc("symmetric_state_squeeze_key",
              std::make_unique<Symmetric::StateSqueezeKey>(Ctx));
  addHostFunc("symmetric_state_max_tag_len",
              std::make_unique<Symmetric::StateMaxTagLen>(Ctx));
  addHostFunc("symmetric_state_encrypt",
              std::make_unique<Symmetric::StateEncrypt>(Ctx));
  addHostFunc("symmetric_state_encrypt_detached",
              std::make_unique<Symmetric::StateEncryptDetached>(Ctx));
  addHostFunc("symmetric_state_decrypt",
              std::make_unique<Symmetric::StateDecrypt>(Ctx));
  addHostFunc("symmetric_state_decrypt_detached",
              std::make_unique<Symmetric::StateDecryptDetached>(Ctx));
  addHostFunc("symmetric_state_ratchet",
              std::make_unique<Symmetric::StateRatchet>(Ctx));
  addHostFunc("symmetric_tag_len", std::make_unique<Symmetric::TagLen>(Ctx));
  addHostFunc("symmetric_tag_pull", std::make_unique<Symmetric::TagPull>(Ctx));
  addHostFunc("symmetric_tag_verify",
              std::make_unique<Symmetric::TagVerify>(Ctx));
  addHostFunc("symmetric_tag_close",
              std::make_unique<Symmetric::TagClose>(Ctx));

  // asymmetric

  addHostFunc("keypair_generate",
              std::make_unique<AsymmetricCommon::KeypairGenerate>(Ctx));
  addHostFunc("keypair_import",
              std::make_unique<AsymmetricCommon::KeypairImport>(Ctx));
  addHostFunc("keypair_generate_managed",
              std::make_unique<AsymmetricCommon::KeypairGenerateManaged>(Ctx));
  addHostFunc("keypair_store_managed",
              std::make_unique<AsymmetricCommon::KeypairStoreManaged>(Ctx));
  addHostFunc("keypair_replace_managed",
              std::make_unique<AsymmetricCommon::KeypairReplaceManaged>(Ctx));
  addHostFunc("keypair_id", std::make_unique<AsymmetricCommon::KeypairId>(Ctx));
  addHostFunc("keypair_from_id",
              std::make_unique<AsymmetricCommon::KeypairFromId>(Ctx));
  addHostFunc("keypair_from_pk_and_sk",
              std::make_unique<AsymmetricCommon::KeypairFromPkAndSk>(Ctx));
  addHostFunc("keypair_export",
              std::make_unique<AsymmetricCommon::KeypairExport>(Ctx));
  addHostFunc("keypair_publickey",
              std::make_unique<AsymmetricCommon::KeypairPublickey>(Ctx));
  addHostFunc("keypair_secretkey",
              std::make_unique<AsymmetricCommon::KeypairSecretkey>(Ctx));
  addHostFunc("keypair_close",
              std::make_unique<AsymmetricCommon::KeypairClose>(Ctx));
  addHostFunc("publickey_import",
              std::make_unique<AsymmetricCommon::PublickeyImport>(Ctx));
  addHostFunc("publickey_export",
              std::make_unique<AsymmetricCommon::PublickeyExport>(Ctx));
  addHostFunc("publickey_verify",
              std::make_unique<AsymmetricCommon::PublickeyVerify>(Ctx));
  addHostFunc("publickey_from_secretkey",
              std::make_unique<AsymmetricCommon::PublickeyFromSecretkey>(Ctx));
  addHostFunc("publickey_close",
              std::make_unique<AsymmetricCommon::PublickeyClose>(Ctx));
  addHostFunc("secretkey_import",
              std::make_unique<AsymmetricCommon::SecretkeyImport>(Ctx));
  addHostFunc("secretkey_export",
              std::make_unique<AsymmetricCommon::SecretkeyExport>(Ctx));
  addHostFunc("secretkey_close",
              std::make_unique<AsymmetricCommon::SecretkeyClose>(Ctx));

  // kx
  addHostFunc("kx_dh", std::make_unique<Kx::Dh>(Ctx));
  addHostFunc("kx_encapsulate", std::make_unique<Kx::Encapsulate>(Ctx));
  addHostFunc("kx_decapsulate", std::make_unique<Kx::Decapsulate>(Ctx));

  // signature
  addHostFunc("signature_export", std::make_unique<Signatures::Export>(Ctx));
  addHostFunc("signature_import", std::make_unique<Signatures::Import>(Ctx));
  addHostFunc("signature_state_open",
              std::make_unique<Signatures::StateOpen>(Ctx));
  addHostFunc("signature_state_update",
              std::make_unique<Signatures::StateUpdate>(Ctx));
  addHostFunc("signature_state_sign",
              std::make_unique<Signatures::StateSign>(Ctx));
  addHostFunc("signature_state_close",
              std::make_unique<Signatures::StateClose>(Ctx));
  addHostFunc("signature_verification_state_open",
              std::make_unique<Signatures::VerificationStateOpen>(Ctx));
  addHostFunc("signature_verification_state_update",
              std::make_unique<Signatures::VerificationStateUpdate>(Ctx));
  addHostFunc("signature_verification_state_verify",
              std::make_unique<Signatures::VerificationStateVerify>(Ctx));
  addHostFunc("signature_verification_state_close",
              std::make_unique<Signatures::VerificationStateClose>(Ctx));
  addHostFunc("signature_close", std::make_unique<Signatures::Close>(Ctx));
}

} // namespace Host
} // namespace WasmEdge