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
    return RsaPkcs12048SHA256::Signature::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PKCS1_2048_SHA384:
    return RsaPkcs12048SHA384::Signature::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PKCS1_2048_SHA512:
    return RsaPkcs12048SHA512::Signature::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PKCS1_3072_SHA384:
    return RsaPkcs13072SHA384::Signature::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PKCS1_3072_SHA512:
    return RsaPkcs13072SHA512::Signature::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PKCS1_4096_SHA512:
    return RsaPkcs14096SHA512::Signature::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PSS_2048_SHA256:
    return RsaPss2048SHA256::Signature::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PSS_2048_SHA384:
    return RsaPss2048SHA384::Signature::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PSS_2048_SHA512:
    return RsaPss2048SHA512::Signature::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PSS_3072_SHA384:
    return RsaPss3072SHA384::Signature::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PSS_3072_SHA512:
    return RsaPss3072SHA512::Signature::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PSS_4096_SHA512:
    return RsaPss4096SHA512::Signature::import(Encoded, Encoding);
  default:
    assumingUnreachable();
  }
}
} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
