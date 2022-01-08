// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/asymmetric_common/secretkey.h"
namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Asymmetric {
WasiCryptoExpect<SecretKey>
secretKeyImport(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
                Span<const uint8_t> Encoded,
                __wasi_secretkey_encoding_e_t Encoding) {
  switch (AlgType) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES: {
    auto Alg = tryFrom<SignatureAlgorithm>(AlgStr);
    if (!Alg) {
      return WasiCryptoUnexpect(Alg);
    }

    auto Res = Signatures::SecretKey::import(*Alg, Encoded, Encoding);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }

    return std::move(*Res);
  }
  case __WASI_ALGORITHM_TYPE_KEY_EXCHANGE: {
    auto Alg = tryFrom<KxAlgorithm>(AlgStr);
    if (!Alg) {
      return WasiCryptoUnexpect(Alg);
    }

    auto Res = Kx::SecretKey::import(*Alg, Encoded, Encoding);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }

    return std::move(*Res);
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
}

WasiCryptoExpect<std::vector<uint8_t>>
secretKeyExportData(SecretKey SecretKey,
                    __wasi_secretkey_encoding_e_t SkEncoding) {
  return std::visit(
      Overloaded{
          [SkEncoding](auto &&Sk) -> WasiCryptoExpect<std::vector<uint8_t>> {
            return Sk->exportData(SkEncoding);
          }},
      SecretKey);
}

WasiCryptoExpect<PublicKey> secretKeyPublicKey(SecretKey SecretKey) {
  return std::visit(Overloaded{[](auto &&Sk) -> WasiCryptoExpect<PublicKey> {
                      return Sk->publicKey();
                    }},
                    SecretKey);
}
} // namespace Asymmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
