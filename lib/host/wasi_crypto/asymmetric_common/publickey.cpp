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

WasiCryptoExpect<KxPublickey> PublicKey::asKxPublicKey() {
  auto *Result = std::get_if<KxPublickey>(&Inner);
  if (Result == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
  }

  return std::move(*Result);
}

WasiCryptoExpect<PublicKey>
PublicKey::import(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
                  Span<uint8_t const> Encoded,
                  __wasi_publickey_encoding_e_t Encoding) {
  switch (AlgType) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES:
    return SignaturePublicKey::import();
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
