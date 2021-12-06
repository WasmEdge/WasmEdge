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


} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
