// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/keypair.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<SignatureKeyPair>
SignatureKeyPair::generate(SignatureAlgorithm Alg,
                           std::optional<SignatureOptions> Options) {
  switch (family(Alg)) {
  case SignatureAlgorithmFamily::ECDSA: {
    auto Res = EcdsaSignatureKeyPair::generate(Alg, Options);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }
    return SignatureKeyPair{*Res};
  }
  case SignatureAlgorithmFamily::EdDSA: {
    auto Res = EddsaSignatureKeyPair::generate(Alg, Options);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }
    return SignatureKeyPair{*Res};
  }
  case SignatureAlgorithmFamily::RSA: {
    auto Res = RsaSignatureKeyPair::generate(Alg, Options);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }
    return SignatureKeyPair{*Res};
  }
  }
}

WasiCryptoExpect<SignatureKeyPair>
SignatureKeyPair::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                         __wasi_keypair_encoding_e_t Encoding) {
  switch (family(Alg)) {
  case SignatureAlgorithmFamily::ECDSA: {
    auto Res = EcdsaSignatureKeyPair::import(Alg, Encoded, Encoding);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }
    return SignatureKeyPair{*Res};
  }
  case SignatureAlgorithmFamily::EdDSA: {
    auto Res = EddsaSignatureKeyPair::import(Alg, Encoded, Encoding);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }
    return SignatureKeyPair{*Res};
  }
  case SignatureAlgorithmFamily::RSA: {
    auto Res = RsaSignatureKeyPair::import(Alg, Encoded, Encoding);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }
    return SignatureKeyPair{*Res};
  }
  }
}

WasiCryptoExpect<std::vector<uint8_t>>
SignatureKeyPair::exportData(__wasi_keypair_encoding_e_t Encoding) {
  return std::visit(
      Overloaded{
          [&Encoding](auto KeyPair) -> WasiCryptoExpect<std::vector<uint8_t>> {
            auto Res = KeyPair.exportData(Encoding);
            if (!Res) {
              return WasiCryptoUnexpect(Res);
            }
            return *Res;
          }},
      Inner);
}

WasiCryptoExpect<SignaturePublicKey> SignatureKeyPair::publicKey() {
  return std::visit(
      Overloaded{[](auto KeyPair) -> WasiCryptoExpect<SignaturePublicKey> {
        auto Res = KeyPair.publicKey();
        if (!Res) {
          return WasiCryptoUnexpect(Res);
        }
        return SignaturePublicKey{*Res};
      }},
      Inner);
}

WasiCryptoExpect<SignatureSecretKey> SignatureKeyPair::secretKey() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
