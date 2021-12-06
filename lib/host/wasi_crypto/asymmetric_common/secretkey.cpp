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
  case __WASI_ALGORITHM_TYPE_SIGNATURES: {
    auto Alg = tryFrom<SignatureAlgorithm>(AlgStr);
    if (!Alg) {
      return WasiCryptoUnexpect(Alg);
    }

    auto Res = SignatureSecretKey::import(*Alg, Encoded, Encoding);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }

    return SecretKey{*Res};
  }
  case __WASI_ALGORITHM_TYPE_KEY_EXCHANGE: {
    auto Alg = tryFrom<KxAlgorithm>(AlgStr);
    if (!Alg) {
      return WasiCryptoUnexpect(Alg);
    }

    auto Res = KxSecretKey::import(*Alg, Encoded, Encoding);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }

    return SecretKey{*Res};
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  }
}

WasiCryptoExpect<std::vector<uint8_t>>
SecretKey::exportData(__wasi_secretkey_encoding_e_t SkEncoding) {
  return std::visit(
      Overloaded{
          [SkEncoding](auto &Sk) -> WasiCryptoExpect<std::vector<uint8_t>> {
            return Sk.inner()->locked([SkEncoding](auto &Inner) {
              return Inner->exportData(SkEncoding);
            });
          }},
      Inner);
}

WasiCryptoExpect<PublicKey> SecretKey::publicKey() {
  return std::visit(Overloaded{[](auto &Sk) -> WasiCryptoExpect<PublicKey> {
                      auto Res = Sk.inner()->locked(
                          [](auto &Inner) { return Inner->publicKey(); });
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
