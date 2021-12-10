// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/keypair.h"
#include "host/wasi_crypto/signature/ecdsa.h"
#include "host/wasi_crypto/signature/eddsa.h"
#include "host/wasi_crypto/signature/rsa.h"

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
    return SignatureKeyPair{std::move(*Res)};
  }
  case SignatureAlgorithmFamily::EdDSA: {
    auto Res = EddsaSignatureKeyPair::generate(Alg, Options);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }
    return SignatureKeyPair{std::move(*Res)};
  }
  case SignatureAlgorithmFamily::RSA: {
    auto Res = RsaSignatureKeyPair::generate(Alg, Options);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }
    return SignatureKeyPair{std::move(*Res)};
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
    return SignatureKeyPair{std::move(*Res)};
  }
  case SignatureAlgorithmFamily::EdDSA: {
    auto Res = EddsaSignatureKeyPair::import(Alg, Encoded, Encoding);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }
    return SignatureKeyPair{std::move(*Res)};
  }
  case SignatureAlgorithmFamily::RSA: {
    auto Res = RsaSignatureKeyPair::import(Alg, Encoded, Encoding);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }
    return SignatureKeyPair{std::move(*Res)};
  }
  }
}


} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
