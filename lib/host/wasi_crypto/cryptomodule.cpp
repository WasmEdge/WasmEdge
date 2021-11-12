// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/cryptomodule.h"
#include "host/wasi_crypto/asymmetric_common/func.h"
#include "host/wasi_crypto/common/func.h"
#include "host/wasi_crypto/key_exchange/func.h"
#include "host/wasi_crypto/symmetric/func.h"

namespace WasmEdge {
namespace Host {

WasiCryptoModule::WasiCryptoModule() : ImportObject("wasi_ephemeral_crypto") {
  using namespace WASICrypto;
  // common
  addHostFunc("array_output_len",
              std::make_unique<Common::ArrayOutputLen>(CommonCtx));
  addHostFunc("array_output_pull",
              std::make_unique<Common::ArrayOutputPull>(CommonCtx));
  addHostFunc("options_open", std::make_unique<Common::OptionsOpen>(CommonCtx));
  addHostFunc("options_close",
              std::make_unique<Common::OptionsClose>(CommonCtx));
  addHostFunc("options_set", std::make_unique<Common::OptionsSet>(CommonCtx));
  addHostFunc("options_set_u64",
              std::make_unique<Common::OptionsSetU64>(CommonCtx));
  addHostFunc("options_set_guest_buffer",
              std::make_unique<Common::OptionsSetGuestBuffer>(CommonCtx));
  addHostFunc("secrets_manager_open",
              std::make_unique<Common::SecretsMangerOpen>(CommonCtx));
  addHostFunc("secrets_manager_close",
              std::make_unique<Common::SecretsMangerClose>(CommonCtx));
  addHostFunc("secrets_manager_invalidate",
              std::make_unique<Common::SecretsMangerInvalidate>(CommonCtx));

  // symmetric
  addHostFunc("symmetric_key_generate",
              std::make_unique<Symmetric::KeyGenerate>(SymmetricCtx));
  addHostFunc("symmetric_key_import",
              std::make_unique<Symmetric::KeyImport>(SymmetricCtx));
  addHostFunc("symmetric_key_export",
              std::make_unique<Symmetric::KeyExport>(SymmetricCtx));
  addHostFunc("symmetric_key_close",
              std::make_unique<Symmetric::KeyClose>(SymmetricCtx));
  addHostFunc("symmetric_key_generate_managed",
              std::make_unique<Symmetric::KeyGenerateManaged>(SymmetricCtx));
  addHostFunc("symmetric_key_store_managed",
              std::make_unique<Symmetric::KeyStoreManaged>(SymmetricCtx));
  addHostFunc("symmetric_key_replace_managed",
              std::make_unique<Symmetric::KeyReplaceManaged>(SymmetricCtx));
  addHostFunc("symmetric_key_id",
              std::make_unique<Symmetric::KeyId>(SymmetricCtx));
  addHostFunc("symmetric_key_from_id",
              std::make_unique<Symmetric::KeyFromId>(SymmetricCtx));
  addHostFunc("symmetric_state_open",
              std::make_unique<Symmetric::StateOpen>(SymmetricCtx));
  addHostFunc("symmetric_state_options_get",
              std::make_unique<Symmetric::StateOptionsGet>(SymmetricCtx));
  addHostFunc("symmetric_state_options_getU64",
              std::make_unique<Symmetric::StateOptionsGetU64>(SymmetricCtx));
  addHostFunc("symmetric_state_close",
              std::make_unique<Symmetric::StateClose>(SymmetricCtx));
  addHostFunc("symmetric_state_absorb",
              std::make_unique<Symmetric::StateAbsorb>(SymmetricCtx));
  addHostFunc("symmetric_state_squeeze",
              std::make_unique<Symmetric::StateSqueeze>(SymmetricCtx));
  addHostFunc("symmetric_state_squeeze_tag",
              std::make_unique<Symmetric::StateSqueezeTag>(SymmetricCtx));
  addHostFunc("symmetric_state_squeeze_key",
              std::make_unique<Symmetric::StateSqueezeKey>(SymmetricCtx));
  addHostFunc("symmetric_state_max_tag_len",
              std::make_unique<Symmetric::StateMaxTagLen>(SymmetricCtx));
  addHostFunc("symmetric_state_encrypt",
              std::make_unique<Symmetric::StateEncrypt>(SymmetricCtx));
  addHostFunc("symmetric_state_encrypt_detached",
              std::make_unique<Symmetric::StateEncryptDetached>(SymmetricCtx));
  addHostFunc("symmetric_state_decrypt",
              std::make_unique<Symmetric::StateDecrypt>(SymmetricCtx));
  addHostFunc("symmetric_state_decrypt_detached",
              std::make_unique<Symmetric::StateDecryptDetached>(SymmetricCtx));
  addHostFunc("symmetric_state_ratchet",
              std::make_unique<Symmetric::StateRatchet>(SymmetricCtx));
  addHostFunc("symmetric_tag_len",
              std::make_unique<Symmetric::TagLen>(SymmetricCtx));
  addHostFunc("symmetric_tag_pull",
              std::make_unique<Symmetric::TagPull>(SymmetricCtx));
  addHostFunc("symmetric_tag_verify",
              std::make_unique<Symmetric::TagVerify>(SymmetricCtx));
  addHostFunc("symmetric_tag_close",
              std::make_unique<Symmetric::TagClose>(SymmetricCtx));

  // asymmetric

  addHostFunc(
      "keypair_generate",
      std::make_unique<AsymmetricCommon::KeypairGenerate>(AsymmetricCtx));
  addHostFunc("keypair_import",
              std::make_unique<AsymmetricCommon::KeypairImport>(AsymmetricCtx));
  addHostFunc("keypair_generate_managed",
              std::make_unique<AsymmetricCommon::KeypairGenerateManaged>(
                  AsymmetricCtx));
  addHostFunc(
      "keypair_store_managed",
      std::make_unique<AsymmetricCommon::KeypairStoreManaged>(AsymmetricCtx));
  addHostFunc(
      "keypair_replace_managed",
      std::make_unique<AsymmetricCommon::KeypairReplaceManaged>(AsymmetricCtx));
  addHostFunc("keypair_id",
              std::make_unique<AsymmetricCommon::KeypairId>(AsymmetricCtx));
  addHostFunc("keypair_from_id",
              std::make_unique<AsymmetricCommon::KeypairFromId>(AsymmetricCtx));
  addHostFunc(
      "keypair_from_pk_and_sk",
      std::make_unique<AsymmetricCommon::KeypairFromPkAndSk>(AsymmetricCtx));
  addHostFunc("keypair_export",
              std::make_unique<AsymmetricCommon::KeypairExport>(AsymmetricCtx));
  addHostFunc(
      "keypair_publickey",
      std::make_unique<AsymmetricCommon::KeypairPublickey>(AsymmetricCtx));
  addHostFunc(
      "keypair_secretkey",
      std::make_unique<AsymmetricCommon::KeypairSecretkey>(AsymmetricCtx));
  addHostFunc("keypair_close",
              std::make_unique<AsymmetricCommon::KeypairClose>(AsymmetricCtx));
  addHostFunc(
      "publickey_import",
      std::make_unique<AsymmetricCommon::PublickeyImport>(AsymmetricCtx));
  addHostFunc(
      "publickey_export",
      std::make_unique<AsymmetricCommon::PublickeyExport>(AsymmetricCtx));
  addHostFunc(
      "publickey_verify",
      std::make_unique<AsymmetricCommon::PublickeyVerify>(AsymmetricCtx));
  addHostFunc("publickey_from_secretkey",
              std::make_unique<AsymmetricCommon::PublickeyFromSecretkey>(
                  AsymmetricCtx));
  addHostFunc(
      "publickey_close",
      std::make_unique<AsymmetricCommon::PublickeyClose>(AsymmetricCtx));
  addHostFunc(
      "publickey_import",
      std::make_unique<AsymmetricCommon::PublickeyImport>(AsymmetricCtx));
  addHostFunc(
      "publickey_export",
      std::make_unique<AsymmetricCommon::PublickeyExport>(AsymmetricCtx));
  addHostFunc(
      "publickey_verify",
      std::make_unique<AsymmetricCommon::PublickeyVerify>(AsymmetricCtx));
  addHostFunc("publickey_from_secretkey",
              std::make_unique<AsymmetricCommon::PublickeyFromSecretkey>(
                  AsymmetricCtx));
  addHostFunc(
      "publickey_close",
      std::make_unique<AsymmetricCommon::PublickeyClose>(AsymmetricCtx));
  addHostFunc(
      "secretkey_import",
      std::make_unique<AsymmetricCommon::SecretkeyImport>(AsymmetricCtx));
  addHostFunc(
      "secretkey_export",
      std::make_unique<AsymmetricCommon::SecretkeyExport>(AsymmetricCtx));
  addHostFunc(
      "secretkey_close",
      std::make_unique<AsymmetricCommon::SecretkeyClose>(AsymmetricCtx));

  // kx
  addHostFunc("kx_dh", std::make_unique<Kx::Dh>(KxCtx));
  addHostFunc("kx_encapsulate", std::make_unique<Kx::Encapsulate>(KxCtx));
  addHostFunc("kx_decapsulate", std::make_unique<Kx::Decapsulate>(KxCtx));
}

} // namespace Host
} // namespace WasmEdge