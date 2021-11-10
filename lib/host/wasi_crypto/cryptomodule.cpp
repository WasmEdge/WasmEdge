// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/cryptomodule.h"
#include "host/wasi_crypto/common/func.h"
#include "host/wasi_crypto/symmetric/func.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {

WasiCryptoModule::WasiCryptoModule() : ImportObject("wasi_ephemeral_crypto") {
  // common
  addHostFunc("array_output_len",
              std::make_unique<WasiCryptoCommonArrayOutputLen>(CommonCtx));
  addHostFunc("array_output_pull",
              std::make_unique<WasiCryptoCommonArrayOutputPull>(CommonCtx));
  addHostFunc("options_open",
              std::make_unique<WasiCryptoCommonOptionsOpen>(CommonCtx));
  addHostFunc("options_close",
              std::make_unique<WasiCryptoCommonOptionsClose>(CommonCtx));
  addHostFunc("options_set",
              std::make_unique<WasiCryptoCommonOptionsSet>(CommonCtx));
  addHostFunc("options_set_u64",
              std::make_unique<WasiCryptoCommonOptionsSetU64>(CommonCtx));
  addHostFunc(
      "options_set_guest_buffer",
      std::make_unique<WasiCryptoCommonOptionsSetGuestBuffer>(CommonCtx));
  addHostFunc("secrets_manager_open",
              std::make_unique<WasiCryptoSecretsMangerOpen>(CommonCtx));
  addHostFunc("secrets_manager_close",
              std::make_unique<WasiCryptoSecretsMangerClose>(CommonCtx));
  addHostFunc("secrets_manager_invalidate",
              std::make_unique<WasiCryptoSecretsMangerInvalidate>(CommonCtx));

  // symmetric
  addHostFunc("symmetric_key_generate",
              std::make_unique<WasiSymmetricKeyGenerate>(SymmetricCtx));
  addHostFunc("symmetric_key_import",
              std::make_unique<WasiSymmetricKeyImport>(SymmetricCtx));
  addHostFunc("symmetric_key_export",
              std::make_unique<WasiSymmetricKeyExport>(SymmetricCtx));
  addHostFunc("symmetric_key_close",
              std::make_unique<WasiSymmetricKeyClose>(SymmetricCtx));
  addHostFunc("symmetric_key_generate_managed",
              std::make_unique<WasiSymmetricKeyGenerateManaged>(SymmetricCtx));
  addHostFunc("symmetric_key_store_managed",
              std::make_unique<WasiSymmetricKeyStoreManaged>(SymmetricCtx));
  addHostFunc("symmetric_key_replace_managed",
              std::make_unique<WasiSymmetricKeyReplaceManaged>(SymmetricCtx));
  addHostFunc("symmetric_key_id",
              std::make_unique<WasiSymmetricKeyId>(SymmetricCtx));
  addHostFunc("symmetric_key_from_id",
              std::make_unique<WasiSymmetricKeyFromId>(SymmetricCtx));
  addHostFunc("symmetric_state_open",
              std::make_unique<WasiSymmetricStateOpen>(SymmetricCtx));
  addHostFunc("symmetric_state_options_get",
              std::make_unique<WasiSymmetricStateOptionsGet>(SymmetricCtx));
  addHostFunc("symmetric_state_options_getU64",
              std::make_unique<WasiSymmetricStateOptionsGetU64>(SymmetricCtx));
  addHostFunc("symmetric_state_close",
              std::make_unique<WasiSymmetricStateClose>(SymmetricCtx));
  addHostFunc("symmetric_state_absorb",
              std::make_unique<WasiSymmetricStateAbsorb>(SymmetricCtx));
  addHostFunc("symmetric_state_squeeze",
              std::make_unique<WasiSymmetricStateSqueeze>(SymmetricCtx));
  addHostFunc("symmetric_state_squeeze_tag",
              std::make_unique<WasiSymmetricStateSqueezeTag>(SymmetricCtx));
  addHostFunc("symmetric_state_squeeze_key",
              std::make_unique<WasiSymmetricStateSqueezeKey>(SymmetricCtx));
  addHostFunc("symmetric_state_max_tag_len",
              std::make_unique<WasiSymmetricStateMaxTagLen>(SymmetricCtx));
  addHostFunc("symmetric_state_encrypt",
              std::make_unique<WasiSymmetricStateEncrypt>(SymmetricCtx));
  addHostFunc(
      "symmetric_state_encrypt_detached",
      std::make_unique<WasiSymmetricStateEncryptDetached>(SymmetricCtx));
  addHostFunc("symmetric_state_decrypt",
              std::make_unique<WasiSymmetricStateDecrypt>(SymmetricCtx));
  addHostFunc(
      "symmetric_state_decrypt_detached",
      std::make_unique<WasiSymmetricStateDecryptDetached>(SymmetricCtx));
  addHostFunc("symmetric_state_ratchet",
              std::make_unique<WasiSymmetricStateRatchet>(SymmetricCtx));
  addHostFunc("symmetric_tag_len",
              std::make_unique<WasiSymmetricTagLen>(SymmetricCtx));
  addHostFunc("symmetric_tag_pull",
              std::make_unique<WasiSymmetricTagPull>(SymmetricCtx));
  addHostFunc("symmetric_tag_verify",
              std::make_unique<WasiSymmetricTagVerify>(SymmetricCtx));
  addHostFunc("symmetric_tag_close",
              std::make_unique<WasiSymmetricTagClose>(SymmetricCtx));
}

} // namespace Host
} // namespace WasmEdge