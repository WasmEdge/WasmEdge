// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/util.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

enum class SignatureAlgorithm {
  ECDSA_P256_SHA256,
  ECDSA_K256_SHA256,
  Ed25519,
  RSA_PKCS1_2048_SHA256,
  RSA_PKCS1_2048_SHA384,
  RSA_PKCS1_2048_SHA512,
  RSA_PKCS1_3072_SHA384,
  RSA_PKCS1_3072_SHA512,
  RSA_PKCS1_4096_SHA512,
  RSA_PSS_2048_SHA256,
  RSA_PSS_2048_SHA384,
  RSA_PSS_2048_SHA512,
  RSA_PSS_3072_SHA384,
  RSA_PSS_3072_SHA512,
  RSA_PSS_4096_SHA512,
};

template <>
constexpr WasiCryptoExpect<SignatureAlgorithm>
tryFrom(std::string_view AlgStr) noexcept {
  if (AlgStr == "ECDSA_P256_SHA256") {
    return SignatureAlgorithm::ECDSA_P256_SHA256;
  }
  if (AlgStr == "ECDSA_K256_SHA256") {
    return SignatureAlgorithm::ECDSA_K256_SHA256;
  }
  if (AlgStr == "Ed25519") {
    return SignatureAlgorithm::Ed25519;
  }
  if (AlgStr == "RSA_PKCS1_2048_SHA256") {
    return SignatureAlgorithm::RSA_PKCS1_2048_SHA256;
  }
  if (AlgStr == "RSA_PKCS1_2048_SHA384") {
    return SignatureAlgorithm::RSA_PKCS1_2048_SHA384;
  }
  if (AlgStr == "RSA_PKCS1_2048_SHA512") {
    return SignatureAlgorithm::RSA_PKCS1_2048_SHA512;
  }
  if (AlgStr == "RSA_PKCS1_3072_SHA384") {
    return SignatureAlgorithm::RSA_PKCS1_3072_SHA384;
  }
  if (AlgStr == "RSA_PKCS1_3072_SHA512") {
    return SignatureAlgorithm::RSA_PKCS1_3072_SHA512;
  }
  if (AlgStr == "RSA_PKCS1_4096_SHA512") {
    return SignatureAlgorithm::RSA_PKCS1_4096_SHA512;
  }
  if (AlgStr == "RSA_PSS_2048_SHA256") {
    return SignatureAlgorithm::RSA_PSS_2048_SHA256;
  }
  if (AlgStr == "RSA_PSS_2048_SHA384") {
    return SignatureAlgorithm::RSA_PSS_2048_SHA384;
  }
  if (AlgStr == "RSA_PSS_2048_SHA512") {
    return SignatureAlgorithm::RSA_PSS_2048_SHA512;
  }
  if (AlgStr == "RSA_PSS_3072_SHA384") {
    return SignatureAlgorithm::RSA_PSS_3072_SHA384;
  }
  if (AlgStr == "RSA_PSS_3072_SHA512") {
    return SignatureAlgorithm::RSA_PSS_3072_SHA512;
  }
  if (AlgStr == "RSA_PSS_4096_SHA512") {
    return SignatureAlgorithm::RSA_PSS_4096_SHA512;
  }
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
}

enum class SignatureAlgorithmFamily { ECDSA, EdDSA, RSA };

 constexpr WasiCryptoExpect<SignatureAlgorithmFamily>
family(SignatureAlgorithm Alg) {
  switch (Alg) {
  case SignatureAlgorithm::ECDSA_P256_SHA256:
  case SignatureAlgorithm::ECDSA_K256_SHA256:
    return SignatureAlgorithmFamily::ECDSA;
  case SignatureAlgorithm::Ed25519:
    return SignatureAlgorithmFamily::EdDSA;
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
    return SignatureAlgorithmFamily::RSA;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
