// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/secretkey.h"
#include "host/wasi_crypto/signature/ecdsa.h"
#include "host/wasi_crypto/signature/eddsa.h"
#include "host/wasi_crypto/signature/rsa.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

WasiCryptoExpect<std::unique_ptr<SecretKey>>
SecretKey::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                  __wasi_secretkey_encoding_e_t Encoding) {
  switch (Alg) {
  case SignatureAlgorithm::ECDSA_P256_SHA256:
    return EcdsaP256::SecretKey::import(Encoded, Encoding);
  case SignatureAlgorithm::ECDSA_K256_SHA256:
    return EcdsaK256::SecretKey::import(Encoded, Encoding);
  case SignatureAlgorithm::Ed25519:
    return EddsaSecretKey::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PKCS1_2048_SHA256:
    return RsaPkcs12048SHA256::SecretKey::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PKCS1_2048_SHA384:
    return RsaPkcs12048SHA384::SecretKey::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PKCS1_2048_SHA512:
    return RsaPkcs12048SHA512::SecretKey::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PKCS1_3072_SHA384:
    return RsaPkcs13072SHA384::SecretKey::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PKCS1_3072_SHA512:
    return RsaPkcs13072SHA512::SecretKey::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PKCS1_4096_SHA512:
    return RsaPkcs14096SHA512::SecretKey::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PSS_2048_SHA256:
    return RsaPss2048SHA256::SecretKey::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PSS_2048_SHA384:
    return RsaPss2048SHA384::SecretKey::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PSS_2048_SHA512:
    return RsaPss2048SHA512::SecretKey::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PSS_3072_SHA384:
    return RsaPss3072SHA384::SecretKey::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PSS_3072_SHA512:
    return RsaPss3072SHA512::SecretKey::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PSS_4096_SHA512: {
    return RsaPss4096SHA512::SecretKey::import(Encoded, Encoding);
  }
  default:
    assumingUnreachable();
  }
}
} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
