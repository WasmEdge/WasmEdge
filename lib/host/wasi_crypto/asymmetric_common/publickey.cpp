// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/asymmetric_common/publickey.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<SignaturePublicKey> PublicKey::asSignaturePublicKey() {
  auto *Result = std::get_if<SignaturePublicKey>(&Inner);
  if (Result == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
  }

  return std::move(*Result);
}

WasiCryptoExpect<KxPublicKey> PublicKey::asKxPublicKey() {
  auto *Result = std::get_if<KxPublicKey>(&Inner);
  if (Result == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
  }

  return std::move(*Result);
}

WasiCryptoExpect<PublicKey>
PublicKey::import(__wasi_algorithm_type_e_t AlgType, SignatureAlgorithm Alg,
                  Span<uint8_t const> Encoded,
                  __wasi_publickey_encoding_e_t Encoding) {
  switch (AlgType) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES:
    return SignaturePublicKey::import();
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
}

WasiCryptoExpect<std::vector<uint8_t>>
PublicKey::exportData(__wasi_publickey_encoding_e_t Encoding) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<std::vector<uint8_t>>();
}

WasiCryptoExpect<void> PublicKey::verify(PublicKey Publickey) {
  return {};
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
