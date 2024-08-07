// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "utils/hostfunction.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

using namespace std::literals;

namespace {
std::string toUpper(std::string_view Name) noexcept {
  std::string Ret{Name};
  std::transform(Ret.begin(), Ret.end(), Ret.begin(),
                 [](char C) { return std::toupper(C); });
  return Ret;
}
} // namespace

WasiCryptoExpect<AsymmetricCommon::Algorithm>
tryFrom(__wasi_algorithm_type_e_t AlgType,
        std::string_view RawAlgStr) noexcept {
  std::string AlgStr = toUpper(RawAlgStr);
  // Delegate to sig and kx.
  switch (AlgType) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES: {
    return tryFrom<Signatures::Algorithm>(AlgStr).map([](auto Alg) noexcept {
      return std::visit(
          [](auto Factory) noexcept -> AsymmetricCommon::Algorithm {
            return Factory;
          },
          Alg);
    });
  }
  case __WASI_ALGORITHM_TYPE_KEY_EXCHANGE: {
    return tryFrom<Kx::Algorithm>(AlgStr).map([](auto Alg) noexcept {
      return std::visit(
          [](auto Factory) noexcept -> AsymmetricCommon::Algorithm {
            return Factory;
          },
          Alg);
    });
  }
  case __WASI_ALGORITHM_TYPE_SYMMETRIC:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  default:
    assumingUnreachable();
  }
}

template <>
WasiCryptoExpect<Kx::Algorithm> tryFrom(std::string_view RawAlgStr) noexcept {
  using namespace Kx;
  std::string AlgStr = toUpper(RawAlgStr);
  if (AlgStr == "X25519"sv) {
    return Algorithm{std::in_place_type<X25519>};
  }
  if (AlgStr == "P256-SHA256"sv) {
    return Algorithm{std::in_place_type<EcdsaP256>};
  }
  if (AlgStr == "P384-SHA384"sv) {
    return Algorithm{std::in_place_type<EcdsaP384>};
  }
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
}

template <>
WasiCryptoExpect<Symmetric::Algorithm>
tryFrom(std::string_view RawAlgStr) noexcept {
  using namespace Symmetric;
  std::string AlgStr = toUpper(RawAlgStr);
  if (AlgStr == "SHA-256"sv) {
    return Algorithm{std::in_place_type<Sha256>};
  }
  if (AlgStr == "SHA-512"sv) {
    return Algorithm{std::in_place_type<Sha512>};
  }
  if (AlgStr == "SHA-512/256"sv) {
    return Algorithm{std::in_place_type<Sha512_256>};
  }
  if (AlgStr == "HMAC/SHA-256"sv) {
    return Algorithm{std::in_place_type<HmacSha256>};
  }
  if (AlgStr == "HMAC/SHA-512"sv) {
    return Algorithm{std::in_place_type<HmacSha512>};
  }
  if (AlgStr == "HKDF-EXPAND/SHA-256"sv) {
    return Algorithm{std::in_place_type<HkdfSha256Expand>};
  }
  if (AlgStr == "HKDF-EXTRACT/SHA-256"sv) {
    return Algorithm{std::in_place_type<HkdfSha256Extract>};
  }
  if (AlgStr == "HKDF-EXPAND/SHA-512"sv) {
    return Algorithm{std::in_place_type<HkdfSha512Expand>};
  }
  if (AlgStr == "HKDF-EXTRACT/SHA-512"sv) {
    return Algorithm{std::in_place_type<HkdfSha512Extract>};
  }
  if (AlgStr == "AES-128-GCM"sv) {
    return Algorithm{std::in_place_type<Aes128Gcm>};
  }
  if (AlgStr == "AES-256-GCM"sv) {
    return Algorithm{std::in_place_type<Aes256Gcm>};
  }
  if (AlgStr == "CHACHA20-POLY1305"sv) {
    return Algorithm{std::in_place_type<ChaCha20Poly1305>};
  }
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
}

template <>
WasiCryptoExpect<Signatures::Algorithm>
tryFrom(std::string_view RawAlgStr) noexcept {
  using namespace Signatures;
  std::string AlgStr = toUpper(RawAlgStr);
  if (AlgStr == "ECDSA_P256_SHA256"sv) {
    return Algorithm{std::in_place_type<EcdsaP256>};
  }
  if (AlgStr == "ECDSA_K256_SHA256"sv) {
    return Algorithm{std::in_place_type<EcdsaK256>};
  }
  if (AlgStr == "ECDSA_P384_SHA384"sv) {
    return Algorithm{std::in_place_type<EcdsaP384>};
  }
  if (AlgStr == "ED25519"sv) {
    return Algorithm{std::in_place_type<Eddsa>};
  }
  if (AlgStr == "RSA_PKCS1_2048_SHA256"sv) {
    return Algorithm{std::in_place_type<RSA_PKCS1_2048_SHA256>};
  }
  if (AlgStr == "RSA_PKCS1_2048_SHA384"sv) {
    return Algorithm{std::in_place_type<RSA_PKCS1_2048_SHA384>};
  }
  if (AlgStr == "RSA_PKCS1_2048_SHA512"sv) {
    return Algorithm{std::in_place_type<RSA_PKCS1_2048_SHA512>};
  }
  if (AlgStr == "RSA_PKCS1_3072_SHA384"sv) {
    return Algorithm{std::in_place_type<RSA_PKCS1_3072_SHA384>};
  }
  if (AlgStr == "RSA_PKCS1_3072_SHA512"sv) {
    return Algorithm{std::in_place_type<RSA_PKCS1_3072_SHA512>};
  }
  if (AlgStr == "RSA_PKCS1_4096_SHA512"sv) {
    return Algorithm{std::in_place_type<RSA_PKCS1_4096_SHA512>};
  }
  if (AlgStr == "RSA_PSS_2048_SHA256"sv) {
    return Algorithm{std::in_place_type<RSA_PSS_2048_SHA256>};
  }
  if (AlgStr == "RSA_PSS_2048_SHA384"sv) {
    return Algorithm{std::in_place_type<RSA_PSS_2048_SHA384>};
  }
  if (AlgStr == "RSA_PSS_2048_SHA512"sv) {
    return Algorithm{std::in_place_type<RSA_PSS_2048_SHA512>};
  }
  if (AlgStr == "RSA_PSS_3072_SHA384"sv) {
    return Algorithm{std::in_place_type<RSA_PSS_3072_SHA384>};
  }
  if (AlgStr == "RSA_PSS_3072_SHA512"sv) {
    return Algorithm{std::in_place_type<RSA_PSS_3072_SHA512>};
  }
  if (AlgStr == "RSA_PSS_4096_SHA512"sv) {
    return Algorithm{std::in_place_type<RSA_PSS_4096_SHA512>};
  }
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
