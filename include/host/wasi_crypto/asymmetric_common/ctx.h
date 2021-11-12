// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/asymmetric_common/keypair.h"
#include "host/wasi_crypto/asymmetric_common/publickey.h"
#include "host/wasi_crypto/asymmetric_common/secretkey.h"
#include "host/wasi_crypto/common/ctx.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/handles.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class AsymmetricCommonContext {
public:
  AsymmetricCommonContext(CommonContext &DependencyCtx);

  WasiCryptoExpect<__wasi_keypair_t>
  keypairGenerate(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
                  std::optional<__wasi_options_t> OptOptions);

  WasiCryptoExpect<__wasi_keypair_encoding_e_t>
  keypairImport(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
                Span<uint8_t> Encoded,
                __wasi_keypair_encoding_e_t KeypairEncoding);

  // opt
  WasiCryptoExpect<__wasi_keypair_t>
  keypairGenerateManaged(__wasi_secrets_manager_t SecretsManager,
                         __wasi_algorithm_type_e_t AlgType,
                         std::string_view AlgStr,
                         std::optional<__wasi_options_t> OptOptions);

  WasiCryptoExpect<void>
  keypairStoreManaged(__wasi_secrets_manager_t SecretsManager,
                      __wasi_keypair_t Keypair, uint8_t_ptr KpIdPtr,
                      __wasi_size_t KpIdLen);

  WasiCryptoExpect<__wasi_version_t>
  keypairReplaceManaged(__wasi_secrets_manager_t SecretsManager,
                        __wasi_keypair_t KpOld, __wasi_keypair_t KpNew);

  WasiCryptoExpect<std::tuple<__wasi_size_t, __wasi_version_t>>
  keypairId(__wasi_keypair_t Kp, uint8_t_ptr KpId, __wasi_size_t KpIdMaxLen);

  WasiCryptoExpect<__wasi_keypair_t>
  keypairFromId(__wasi_secrets_manager_t SecretsManager, const_uint8_t_ptr KpId,
                __wasi_size_t KpIdLen, __wasi_version_t KpIdVersion);

  WasiCryptoExpect<__wasi_keypair_t> keypairFromPkAndSk(__wasi_publickey_t Pk,
                                                        __wasi_secretkey_t Sk);
  WasiCryptoExpect<__wasi_array_output_t>
  keypairExport(__wasi_keypair_t Keypair,
                __wasi_keypair_encoding_e_t KeypairEncoding);

  WasiCryptoExpect<__wasi_publickey_t>
  keypairPublickey(__wasi_keypair_t Keypair);

  WasiCryptoExpect<__wasi_secretkey_t>
  keypairSecretkey(__wasi_keypair_t Keypair);

  WasiCryptoExpect<void> keypairClose(__wasi_keypair_t Keypair);

  WasiCryptoExpect<__wasi_publickey_t>
  publickeyImport(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
                  Span<uint8_t> Encoded,
                  __wasi_keypair_encoding_e_t EncodingEnum);

  WasiCryptoExpect<__wasi_array_output_t>
  publicKeyExport(__wasi_publickey_t Pk,
                  __wasi_publickey_encoding_e_t PkEncoding);

  WasiCryptoExpect<void> publickeyVerify(__wasi_publickey_t Pk);

  WasiCryptoExpect<__wasi_publickey_t>
  publickeyFroSecretkey(__wasi_secretkey_t i);

  WasiCryptoExpect<void> publickeyClose(__wasi_publickey_t i);

  WasiCryptoExpect<__wasi_secretkey_t>
  secretkeyImport(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
                  Span<uint8_t> Encoded,
                  __wasi_secretkey_encoding_e_t EncodingEnum);

  WasiCryptoExpect<__wasi_array_output_t>
  secretkeyExport(__wasi_secretkey_t Sk,
                  __wasi_secretkey_encoding_e_t SkEncoding);

  WasiCryptoExpect<void> secretkeyClose(__wasi_secretkey_t Sk);

private:
  CommonContext &CommonCtx;

  HandlesManger<__wasi_keypair_t, Keypair> KeypairManger{0x02};
  HandlesManger<__wasi_keypair_t, PublicKey> PublickeyManger{0x03};
  HandlesManger<__wasi_keypair_t, SecretKey> SecretkeyManger{0x04};
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
