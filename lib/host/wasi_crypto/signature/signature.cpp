// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/signature.h"
#include "host/wasi_crypto/signature/ecdsa.h"
#include "host/wasi_crypto/signature/eddsa.h"
#include "host/wasi_crypto/signature/keypair.h"
#include "host/wasi_crypto/signature/publickey.h"
#include "host/wasi_crypto/signature/rsa.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<Signature>
Signature::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                   __wasi_signature_encoding_e_t Encoding) {
  auto Family = family(Alg);
  if (!Family) {
    return WasiCryptoUnexpect(Family);
  }

  switch (*Family) {
  case SignatureAlgorithmFamily::ECDSA: {
    auto Sig = EcdsaSignature::import(Alg, Encoded, Encoding);
    if (!Sig) {
      return WasiCryptoUnexpect(Sig);
    }
    return std::move(*Sig);
  }
  case SignatureAlgorithmFamily::EdDSA: {
    auto Sig = EddsaSignature::import(Alg, Encoded, Encoding);
    if (!Sig) {
      return WasiCryptoUnexpect(Sig);
    }
    return std::move(*Sig);
  }
  case SignatureAlgorithmFamily::RSA: {
    auto Sig = RsaSignature::import(Alg, Encoded, Encoding);
    if (!Sig) {
      return WasiCryptoUnexpect(Sig);
    }
    return std::move(*Sig);
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
