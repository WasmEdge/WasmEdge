// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/asymmetric_common/secretkey.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<SecretKey>
SecretKey::import(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
                  Span<const uint8_t> Encoded,
                  __wasi_secretkey_encoding_e_t Encoding) {
  switch (AlgType) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  case __WASI_ALGORITHM_TYPE_SYMMETRIC:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  case __WASI_ALGORITHM_TYPE_KEY_EXCHANGE:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
}

WasiCryptoExpect<std::vector<uint8_t>>
SecretKey::exportData(__wasi_secretkey_encoding_e_t SkEncoding) {
  return std::visit(
      Overloaded{
          [&SkEncoding](auto Sk) -> WasiCryptoExpect<std::vector<uint8_t>> {
            return Sk.exportData(SkEncoding);
          }},
      Inner);
}

WasiCryptoExpect<PublicKey> SecretKey::publicKey() {
  return std::visit(Overloaded{[](auto Sk) -> WasiCryptoExpect<PublicKey> {
                      auto Res = Sk.publicKey();
                      if (!Res) {
                        return WasiCryptoUnexpect(Res);
                      }
                      return PublicKey{*Res};
                    }},
                    Inner);
}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
