// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/keypair.h"
#include "host/wasi_crypto/signature/ecdsa.h"
#include "host/wasi_crypto/signature/eddsa.h"
#include "host/wasi_crypto/signature/rsa.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

WasiCryptoExpect<std::unique_ptr<KeyPair>>
KeyPair::generate(SignatureAlgorithm Alg, std::shared_ptr<Options> Options) {
  switch (Alg) {
  case SignatureAlgorithm::ECDSA_P256_SHA256:
    return EcdsaP256::KeyPair::generate(Options);
  case SignatureAlgorithm::ECDSA_K256_SHA256:
    return EcdsaK256::KeyPair::generate(Options);
  case SignatureAlgorithm::Ed25519:
    return EddsaKeyPair::generate(Options);
  case SignatureAlgorithm::RSA_PKCS1_2048_SHA256:
    return RsaPkcs12048SHA256::KeyPair::generate(Options);
  case SignatureAlgorithm::RSA_PKCS1_2048_SHA384:
    return RsaPkcs12048SHA384::KeyPair::generate(Options);
  case SignatureAlgorithm::RSA_PKCS1_2048_SHA512:
    return RsaPkcs12048SHA512::KeyPair::generate(Options);
  case SignatureAlgorithm::RSA_PKCS1_3072_SHA384:
    return RsaPkcs13072SHA384::KeyPair::generate(Options);
  case SignatureAlgorithm::RSA_PKCS1_3072_SHA512:
    return RsaPkcs13072SHA512::KeyPair::generate(Options);
  case SignatureAlgorithm::RSA_PKCS1_4096_SHA512:
    return RsaPkcs14096SHA512::KeyPair::generate(Options);
  case SignatureAlgorithm::RSA_PSS_2048_SHA256:
    return RsaPss2048SHA256::KeyPair::generate(Options);
  case SignatureAlgorithm::RSA_PSS_2048_SHA384:
    return RsaPss2048SHA384::KeyPair::generate(Options);
  case SignatureAlgorithm::RSA_PSS_2048_SHA512:
    return RsaPss2048SHA512::KeyPair::generate(Options);
  case SignatureAlgorithm::RSA_PSS_3072_SHA384:
    return RsaPss3072SHA384::KeyPair::generate(Options);
  case SignatureAlgorithm::RSA_PSS_3072_SHA512:
    return RsaPss3072SHA512::KeyPair::generate(Options);
  case SignatureAlgorithm::RSA_PSS_4096_SHA512:
    return RsaPss4096SHA512::KeyPair::generate(Options);
  default:
    assumingUnreachable();
  }
}

WasiCryptoExpect<std::unique_ptr<KeyPair>>
KeyPair::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                __wasi_keypair_encoding_e_t Encoding) {
  switch (Alg) {
  case SignatureAlgorithm::ECDSA_P256_SHA256:
    return EcdsaP256::KeyPair::import(Encoded, Encoding);
  case SignatureAlgorithm::ECDSA_K256_SHA256:
    return EcdsaK256::KeyPair::import(Encoded, Encoding);
  case SignatureAlgorithm::Ed25519:
    return EddsaKeyPair::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PKCS1_2048_SHA256:
    return RsaPkcs12048SHA256::KeyPair::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PKCS1_2048_SHA384:
    return RsaPkcs12048SHA384::KeyPair::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PKCS1_2048_SHA512:
    return RsaPkcs12048SHA512::KeyPair::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PKCS1_3072_SHA384:
    return RsaPkcs13072SHA384::KeyPair::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PKCS1_3072_SHA512:
    return RsaPkcs13072SHA512::KeyPair::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PKCS1_4096_SHA512:
    return RsaPkcs14096SHA512::KeyPair::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PSS_2048_SHA256:
    return RsaPss2048SHA256::KeyPair::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PSS_2048_SHA384:
    return RsaPss2048SHA384::KeyPair::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PSS_2048_SHA512:
    return RsaPss2048SHA512::KeyPair::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PSS_3072_SHA384:
    return RsaPss3072SHA384::KeyPair::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PSS_3072_SHA512:
    return RsaPss3072SHA512::KeyPair::import(Encoded, Encoding);
  case SignatureAlgorithm::RSA_PSS_4096_SHA512:
    return RsaPss4096SHA512::KeyPair::import(Encoded, Encoding);
  default:
    assumingUnreachable();
  }
}

} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
