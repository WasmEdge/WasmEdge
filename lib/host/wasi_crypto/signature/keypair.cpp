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
    return RsaKeyPair::generate(Alg, Options);
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
    return RsaKeyPair::import(Alg, Encoded, Encoding);
  default:
    assumingUnreachable();
  }
}

} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
