// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/publickey.h"
#include "host/wasi_crypto/signature/ecdsa.h"
#include "host/wasi_crypto/signature/eddsa.h"
#include "host/wasi_crypto/signature/rsa.h"

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

    return SignaturePublicKey{std::move(*Res)};
  }
  case SignatureAlgorithmFamily::EdDSA: {
    auto Res = EddsaSignaturePublicKey::import(Alg, Encoded, Encoding);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }
    return SignaturePublicKey{std::move(*Res)};
  }
  case SignatureAlgorithmFamily::RSA: {
    auto Res = RsaSignaturePublicKey::import(Alg, Encoded, Encoding);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }
    return SignaturePublicKey{std::move(*Res)};
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
