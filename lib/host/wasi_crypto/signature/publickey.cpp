// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/publickey.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<SignaturePublicKey>
SignaturePublicKey::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                           __wasi_publickey_encoding_e_t Encoding) {
  switch (family(Alg)) {
  case SignatureAlgorithmFamily::ECDSA: {
    auto Res = EcdsaSignaturePublicKey::import(Alg, Encoded, Encoding);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }
    return SignaturePublicKey{*Res};
  }
  case SignatureAlgorithmFamily::EdDSA: {
    auto Res = EddsaSignaturePublicKey::import(Alg, Encoded, Encoding);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }
    return SignaturePublicKey{*Res};
  }
  case SignatureAlgorithmFamily::RSA: {
    auto Res = RsaSignaturePublicKey::import(Alg, Encoded, Encoding);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }
    return SignaturePublicKey{*Res};
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
}

WasiCryptoExpect<std::vector<uint8_t>>
SignaturePublicKey::exportData(__wasi_publickey_encoding_e_t PublicKey) {
  return std::visit(
      Overloaded{
          [&PublicKey](auto Pk) -> WasiCryptoExpect<std::vector<uint8_t>> {
            auto Res = Pk.exportData(PublicKey);
            if (!Res) {
              return WasiCryptoUnexpect(Res);
            }
            return *Res;
          }},
      Inner);
}

SignatureAlgorithm SignaturePublicKey::alg() {
  return std::visit(Overloaded{[](auto PK) { return PK.Alg; }}, Inner);
}

WasiCryptoExpect<void> SignaturePublicKey::verify() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
