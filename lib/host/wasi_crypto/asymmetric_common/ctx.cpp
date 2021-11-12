// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/asymmetric_common/ctx.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

namespace {
template <typename... Targs> void dummyCode(Targs &&.../* unused */) {}
} // namespace

AsymmetricCommonContext::AsymmetricCommonContext(CommonContext &DependencyCtx)
    : CommonCtx(DependencyCtx) {}

WasiCryptoExpect<__wasi_keypair_t> AsymmetricCommonContext::keypairGenerate(
    __wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
    std::optional<__wasi_options_t> OptOptions) {
  dummyCode(AlgType, AlgStr, OptOptions);
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<__wasi_keypair_encoding_e_t>
AsymmetricCommonContext::keypairImport(
    __wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
    Span<uint8_t> Encoded, __wasi_keypair_encoding_e_t KeypairEncoding) {
  dummyCode(AlgType, AlgStr, Encoded, KeypairEncoding);
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<__wasi_keypair_t>
AsymmetricCommonContext::keypairGenerateManaged(
    __wasi_secrets_manager_t SecretsManager, __wasi_algorithm_type_e_t AlgType,
    std::string_view AlgStr, std::optional<__wasi_options_t> OptOptions) {
  dummyCode(SecretsManager, AlgType, AlgStr, OptOptions);
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<void> AsymmetricCommonContext::keypairStoreManaged(
    __wasi_secrets_manager_t SecretsManager, __wasi_keypair_t Keypair,
    uint8_t_ptr KpIdPtr, __wasi_size_t KpIdLen) {
  dummyCode(SecretsManager, Keypair, KpIdPtr, KpIdLen);
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<__wasi_version_t>
AsymmetricCommonContext::keypairReplaceManaged(
    __wasi_secrets_manager_t SecretsManager, __wasi_keypair_t KpOld,
    __wasi_keypair_t KpNew) {
  dummyCode(SecretsManager, KpOld, KpNew);
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::tuple<__wasi_size_t, __wasi_version_t>>
AsymmetricCommonContext::keypairId(__wasi_keypair_t Kp, uint8_t_ptr KpId,
                                   __wasi_size_t KpIdMaxLen) {
  dummyCode(Kp, KpId, KpIdMaxLen);
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<__wasi_keypair_t> AsymmetricCommonContext::keypairFromId(
    __wasi_secrets_manager_t SecretsManager, const_uint8_t_ptr KpId,
    __wasi_size_t KpIdLen, __wasi_version_t KpIdVersion) {
  dummyCode(SecretsManager, KpId, KpIdLen, KpIdVersion);
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<__wasi_keypair_t>
AsymmetricCommonContext::keypairFromPkAndSk(__wasi_publickey_t Pk,
                                            __wasi_secretkey_t Sk) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_keypair_t>();
}

WasiCryptoExpect<__wasi_array_output_t> AsymmetricCommonContext::keypairExport(
    __wasi_keypair_t Keypair, __wasi_keypair_encoding_e_t KeypairEncoding) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_array_output_t>();
}

WasiCryptoExpect<__wasi_publickey_t>
AsymmetricCommonContext::keypairPublickey(__wasi_keypair_t Keypair) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_publickey_t>();
}

WasiCryptoExpect<__wasi_secretkey_t>
AsymmetricCommonContext::keypairSecretkey(__wasi_keypair_t Keypair) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_secretkey_t>();
}

WasiCryptoExpect<void>
AsymmetricCommonContext::keypairClose(__wasi_keypair_t Keypair) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}

WasiCryptoExpect<__wasi_publickey_t> AsymmetricCommonContext::publickeyImport(
    __wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
    Span<uint8_t> Encoded, __wasi_keypair_encoding_e_t EncodingEnum) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_publickey_t>();
}

WasiCryptoExpect<__wasi_array_output_t>
AsymmetricCommonContext::publicKeyExport(
    __wasi_publickey_t Pk, __wasi_publickey_encoding_e_t PkEncoding) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_array_output_t>();
}

WasiCryptoExpect<void>
AsymmetricCommonContext::publickeyVerify(__wasi_publickey_t Pk) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}

WasiCryptoExpect<__wasi_publickey_t>
AsymmetricCommonContext::publickeyFroSecretkey(__wasi_secretkey_t i) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_publickey_t>();
}

WasiCryptoExpect<void>
AsymmetricCommonContext::publickeyClose(__wasi_publickey_t i) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}

WasiCryptoExpect<__wasi_secretkey_t> AsymmetricCommonContext::secretkeyImport(
    __wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
    Span<uint8_t> Encoded, __wasi_secretkey_encoding_e_t EncodingEnum) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_secretkey_t>();
}

WasiCryptoExpect<__wasi_array_output_t>
AsymmetricCommonContext::secretkeyExport(
    __wasi_secretkey_t Sk, __wasi_secretkey_encoding_e_t SkEncoding) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_array_output_t>();
}

WasiCryptoExpect<void>
AsymmetricCommonContext::secretkeyClose(__wasi_secretkey_t Sk) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
