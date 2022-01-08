// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/signature.h"
#include "host/wasi_crypto/signature/ecdsa.h"
#include "host/wasi_crypto/signature/eddsa.h"
#include "host/wasi_crypto/signature/rsa.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures{
WasiCryptoExpect<Signature>
Signature::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                  __wasi_signature_encoding_e_t Encoding) {
  auto Family = family(Alg);
  if (!Family) {
    return WasiCryptoUnexpect(Family);
  }

  switch (*Family) {
  case SignatureAlgorithmFamily::ECDSA: {
    return EcdsaSignature::import(Alg, Encoded, Encoding);
  }
  case SignatureAlgorithmFamily::EdDSA: {
    return EddsaSignature::import(Alg, Encoded, Encoding);
  }
  case SignatureAlgorithmFamily::RSA: {
    return RsaSignature::import(Alg, Encoded, Encoding);
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
}
}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
