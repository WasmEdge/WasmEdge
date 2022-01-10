// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/signature.h"
#include "host/wasi_crypto/signature/ecdsa.h"
#include "host/wasi_crypto/signature/eddsa.h"
#include "host/wasi_crypto/signature/rsa.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

WasiCryptoExpect<std::unique_ptr<Signature>>
Signature::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                  __wasi_signature_encoding_e_t Encoding) {
  switch (Alg) {
  case SignatureAlgorithm::ECDSA_P256_SHA256:
    return EcdsaP256::Signature::import(Encoded, Encoding);
  case SignatureAlgorithm::ECDSA_K256_SHA256:
    return EcdsaK256::Signature::import(Encoded, Encoding);
  case SignatureAlgorithm::Ed25519:
    return EddsaSignature::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PKCS1_2048_SHA256:
  case SignatureAlgorithm::RSA_PKCS1_2048_SHA384:
  case SignatureAlgorithm::RSA_PKCS1_2048_SHA512:
  case SignatureAlgorithm::RSA_PKCS1_3072_SHA384:
  case SignatureAlgorithm::RSA_PKCS1_3072_SHA512:
  case SignatureAlgorithm::RSA_PKCS1_4096_SHA512:
  case SignatureAlgorithm::RSA_PSS_2048_SHA256:
  case SignatureAlgorithm::RSA_PSS_2048_SHA384:
  case SignatureAlgorithm::RSA_PSS_2048_SHA512:
  case SignatureAlgorithm::RSA_PSS_3072_SHA384:
  case SignatureAlgorithm::RSA_PSS_3072_SHA512:
  case SignatureAlgorithm::RSA_PSS_4096_SHA512:
    return RsaSignature::import(Alg, Encoded, Encoding);
  default:
    assumingUnreachable();
  }
}
} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
