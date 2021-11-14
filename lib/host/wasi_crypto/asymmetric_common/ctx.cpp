// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/ctx.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

namespace {
template <typename... Targs> void dummyCode(Targs &&.../* unused */) {}
} // namespace

WasiCryptoExpect<__wasi_keypair_t> WasiCryptoContext::keypairGenerate(
    __wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
    std::optional<__wasi_options_t> OptOptions) {
  dummyCode(AlgType, AlgStr, OptOptions);
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<__wasi_keypair_encoding_e_t>
WasiCryptoContext::keypairImport(
    __wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
    Span<uint8_t> Encoded, __wasi_keypair_encoding_e_t KeypairEncoding) {
  dummyCode(AlgType, AlgStr, Encoded, KeypairEncoding);
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<__wasi_keypair_t>
WasiCryptoContext::keypairGenerateManaged(
    __wasi_secrets_manager_t SecretsManager, __wasi_algorithm_type_e_t AlgType,
    std::string_view AlgStr, std::optional<__wasi_options_t> OptOptions) {
  dummyCode(SecretsManager, AlgType, AlgStr, OptOptions);
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<void> WasiCryptoContext::keypairStoreManaged(
    __wasi_secrets_manager_t SecretsManager, __wasi_keypair_t Keypair,
    uint8_t_ptr KpIdPtr, __wasi_size_t KpIdLen) {
  dummyCode(SecretsManager, Keypair, KpIdPtr, KpIdLen);
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<__wasi_version_t>
WasiCryptoContext::keypairReplaceManaged(
    __wasi_secrets_manager_t SecretsManager, __wasi_keypair_t KpOld,
    __wasi_keypair_t KpNew) {
  dummyCode(SecretsManager, KpOld, KpNew);
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::tuple<__wasi_size_t, __wasi_version_t>>
WasiCryptoContext::keypairId(__wasi_keypair_t Kp, uint8_t_ptr KpId,
                                   __wasi_size_t KpIdMaxLen) {
  dummyCode(Kp, KpId, KpIdMaxLen);
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<__wasi_keypair_t> WasiCryptoContext::keypairFromId(
    __wasi_secrets_manager_t SecretsManager, const_uint8_t_ptr KpId,
    __wasi_size_t KpIdLen, __wasi_version_t KpIdVersion) {
  dummyCode(SecretsManager, KpId, KpIdLen, KpIdVersion);
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<__wasi_keypair_t>
WasiCryptoContext::keypairFromPkAndSk(__wasi_publickey_t  Pk,
                                            __wasi_secretkey_t Sk) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_keypair_t>();
}

WasiCryptoExpect<__wasi_array_output_t> WasiCryptoContext::keypairExport(
    __wasi_keypair_t Keypair, __wasi_keypair_encoding_e_t KeypairEncoding) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_array_output_t>();
}

WasiCryptoExpect<__wasi_publickey_t>
WasiCryptoContext::keypairPublickey(__wasi_keypair_t Keypair) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_publickey_t>();
}

WasiCryptoExpect<__wasi_secretkey_t>
WasiCryptoContext::keypairSecretkey(__wasi_keypair_t Keypair) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_secretkey_t>();
}

WasiCryptoExpect<void>
WasiCryptoContext::keypairClose(__wasi_keypair_t Keypair) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}

WasiCryptoExpect<__wasi_publickey_t> WasiCryptoContext::publickeyImport(
    __wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
    Span<uint8_t> Encoded, __wasi_publickey_encoding_e_t EncodingEnum) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_publickey_t>();
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoContext::publickeyExport(
    __wasi_publickey_t Pk, __wasi_publickey_encoding_e_t PkEncoding) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_array_output_t>();
}

WasiCryptoExpect<void>
WasiCryptoContext::publickeyVerify(__wasi_publickey_t Pk) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}

WasiCryptoExpect<__wasi_publickey_t>
WasiCryptoContext::publickeyFroSecretkey(__wasi_secretkey_t i) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_publickey_t>();
}

WasiCryptoExpect<void>
WasiCryptoContext::publickeyClose(__wasi_publickey_t i) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}

WasiCryptoExpect<__wasi_secretkey_t> WasiCryptoContext::secretkeyImport(
    __wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
    Span<uint8_t> Encoded, __wasi_secretkey_encoding_e_t EncodingEnum) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_secretkey_t>();
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoContext::secretkeyExport(
    __wasi_secretkey_t Sk, __wasi_secretkey_encoding_e_t SkEncoding) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_array_output_t>();
}

WasiCryptoExpect<void>
WasiCryptoContext::secretkeyClose(__wasi_secretkey_t Sk) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
