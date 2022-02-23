// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/wasi_crypto/hostfunc.h - HostFunction class definition ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the HostFunction class and some helper functions
/// interact with wasi
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/ctx.h"
#include "host/wasi_crypto/kx/alg.h"
#include "host/wasi_crypto/signatures/alg.h"
#include "host/wasi_crypto/symmetric/alg.h"
#include "host/wasi_crypto/utils/error.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"
#include "wasi_crypto/api.hpp"

#include <optional>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
using namespace std::literals;

namespace detail {
inline std::string toUpper(std::string_view Name) noexcept {
  std::string Ret{Name};
  std::transform(Ret.begin(), Ret.end(), Ret.begin(),
                 [](char C) { return std::toupper(C); });
  return Ret;
}
} // namespace detail

/// The wasi-crypto's base HostFunction class, every wasi-crypto HostFunction
/// should inherit from this class
template <typename T> class HostFunction : public Runtime::HostFunction<T> {
public:
  HostFunction(Context &Ctx) : Runtime::HostFunction<T>(0), Ctx(Ctx) {}

protected:
  Context &Ctx;
};

/// Convert wasi optional type to C++ optional type
constexpr std::optional<__wasi_options_t>
toOptional(const __wasi_opt_options_t Union) noexcept {
  if (Union.tag == __WASI_OPT_OPTIONS_U_SOME) {
    return Union.u.some;
  }
  return std::nullopt;
}

constexpr std::optional<__wasi_symmetric_key_t>
toOptional(const __wasi_opt_symmetric_key_t Union) noexcept {
  if (Union.tag == __WASI_OPT_SYMMETRIC_KEY_U_SOME) {
    return Union.u.some;
  }
  return std::nullopt;
}

/// Cast wasi enum to c++ enum
template <typename T> constexpr WasiCryptoExpect<T> cast(uint64_t) noexcept;

template <typename T> struct WasiRawType {
  using Type = std::underlying_type_t<T>;
};
template <> struct WasiRawType<uint8_t> { using Type = uint8_t; };
template <> struct WasiRawType<uint16_t> { using Type = uint16_t; };
template <> struct WasiRawType<uint32_t> { using Type = uint32_t; };
template <> struct WasiRawType<uint64_t> { using Type = uint64_t; };

template <typename T> using WasiRawTypeT = typename WasiRawType<T>::Type;

template <>
constexpr WasiCryptoExpect<__wasi_algorithm_type_e_t>
cast(uint64_t Encoding) noexcept {
  switch (static_cast<WasiRawTypeT<__wasi_algorithm_type_e_t>>(Encoding)) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES:
  case __WASI_ALGORITHM_TYPE_SYMMETRIC:
  case __WASI_ALGORITHM_TYPE_KEY_EXCHANGE:
    return static_cast<__wasi_algorithm_type_e_t>(Encoding);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <>
constexpr WasiCryptoExpect<__wasi_keypair_encoding_e_t>
cast(uint64_t Encoding) noexcept {
  switch (static_cast<WasiRawTypeT<__wasi_keypair_encoding_e_t>>(Encoding)) {
  case __WASI_KEYPAIR_ENCODING_RAW:
  case __WASI_KEYPAIR_ENCODING_PKCS8:
  case __WASI_KEYPAIR_ENCODING_PEM:
  case __WASI_KEYPAIR_ENCODING_COMPRESSED_PKCS8:
  case __WASI_KEYPAIR_ENCODING_COMPRESSED_PEM:
  case __WASI_KEYPAIR_ENCODING_LOCAL:
    return static_cast<__wasi_keypair_encoding_e_t>(Encoding);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <>
constexpr WasiCryptoExpect<__wasi_publickey_encoding_e_t>
cast(uint64_t Encoding) noexcept {
  switch (static_cast<WasiRawTypeT<__wasi_publickey_encoding_e_t>>(Encoding)) {
  case __WASI_PUBLICKEY_ENCODING_RAW:
  case __WASI_PUBLICKEY_ENCODING_PKCS8:
  case __WASI_PUBLICKEY_ENCODING_PEM:
  case __WASI_PUBLICKEY_ENCODING_SEC:
  case __WASI_PUBLICKEY_ENCODING_COMPRESSED_SEC:
  case __WASI_PUBLICKEY_ENCODING_COMPRESSED_PKCS8:
  case __WASI_PUBLICKEY_ENCODING_COMPRESSED_PEM:
  case __WASI_PUBLICKEY_ENCODING_LOCAL:
    return static_cast<__wasi_publickey_encoding_e_t>(Encoding);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <>
constexpr WasiCryptoExpect<__wasi_secretkey_encoding_e_t>
cast(uint64_t Encoding) noexcept {
  switch (static_cast<WasiRawTypeT<__wasi_secretkey_encoding_e_t>>(Encoding)) {
  case __WASI_SECRETKEY_ENCODING_RAW:
  case __WASI_SECRETKEY_ENCODING_PKCS8:
  case __WASI_SECRETKEY_ENCODING_PEM:
  case __WASI_SECRETKEY_ENCODING_SEC:
  case __WASI_SECRETKEY_ENCODING_LOCAL:
    return static_cast<__wasi_secretkey_encoding_e_t>(Encoding);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <>
constexpr WasiCryptoExpect<__wasi_signature_encoding_e_t>
cast(uint64_t Encoding) noexcept {
  switch (static_cast<WasiRawTypeT<__wasi_signature_encoding_e_t>>(Encoding)) {
  case __WASI_SIGNATURE_ENCODING_RAW:
  case __WASI_SIGNATURE_ENCODING_DER:
    return static_cast<__wasi_signature_encoding_e_t>(Encoding);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

/// Cast c++ size_t to wasi size_t
constexpr WasiCryptoExpect<__wasi_size_t> toWasiSize(size_t Size) noexcept {
  ensureOrReturn(Size <= std::numeric_limits<__wasi_size_t>::max(),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  return static_cast<__wasi_size_t>(Size);
}

/// convert string_view to inner enum Alg representation
template <typename T> WasiCryptoExpect<T> tryFrom(std::string_view) noexcept;

template <>
inline WasiCryptoExpect<Symmetric::Algorithm>
tryFrom(std::string_view RawAlgStr) noexcept {
  std::string AlgStr = detail::toUpper(RawAlgStr);
  if (AlgStr == "HKDF-EXTRACT/SHA-256"sv)
    return Symmetric::Algorithm::HkdfSha256Extract;
  if (AlgStr == "HKDF-EXTRACT/SHA-512"sv)
    return Symmetric::Algorithm::HkdfSha512Extract;
  if (AlgStr == "HKDF-EXPAND/SHA-256"sv)
    return Symmetric::Algorithm::HkdfSha256Expand;
  if (AlgStr == "HKDF-EXPAND/SHA-512"sv)
    return Symmetric::Algorithm::HkdfSha512Expand;
  if (AlgStr == "HMAC/SHA-256"sv)
    return Symmetric::Algorithm::HmacSha256;
  if (AlgStr == "HMAC/SHA-512"sv)
    return Symmetric::Algorithm::HmacSha512;
  if (AlgStr == "SHA-256"sv)
    return Symmetric::Algorithm::Sha256;
  if (AlgStr == "SHA-512"sv)
    return Symmetric::Algorithm::Sha512;
  if (AlgStr == "SHA-512/256"sv)
    return Symmetric::Algorithm::Sha512_256;
  if (AlgStr == "AES-128-GCM"sv)
    return Symmetric::Algorithm::Aes128Gcm;
  if (AlgStr == "AES-256-GCM"sv)
    return Symmetric::Algorithm::Aes256Gcm;
  if (AlgStr == "CHACHA20-POLY1305"sv)
    return Symmetric::Algorithm::ChaCha20Poly1305;
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
}

template <>
inline WasiCryptoExpect<Kx::Algorithm>
tryFrom(std::string_view RawAlgStr) noexcept {
  std::string AlgStr = detail::toUpper(RawAlgStr);
  if (AlgStr == "X25519"sv) {
    return Kx::Algorithm::X25519;
  }
  if (AlgStr == "X25519"sv) {
    return Kx::Algorithm::P256_SHA256;
  }
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
}

template <>
inline WasiCryptoExpect<Signatures::Algorithm>
tryFrom(std::string_view RawAlgStr) noexcept {
  std::string AlgStr = detail::toUpper(RawAlgStr);
  if (AlgStr == "ECDSA_P256_SHA256"sv) {
    return Signatures::Algorithm::ECDSA_P256_SHA256;
  }
  if (AlgStr == "ECDSA_K256_SHA256"sv) {
    return Signatures::Algorithm::ECDSA_K256_SHA256;
  }
  if (AlgStr == "ED25519"sv) {
    return Signatures::Algorithm::Ed25519;
  }
  if (AlgStr == "RSA_PKCS1_2048_SHA256"sv) {
    return Signatures::Algorithm::RSA_PKCS1_2048_SHA256;
  }
  if (AlgStr == "RSA_PKCS1_2048_SHA384"sv) {
    return Signatures::Algorithm::RSA_PKCS1_2048_SHA384;
  }
  if (AlgStr == "RSA_PKCS1_2048_SHA512"sv) {
    return Signatures::Algorithm::RSA_PKCS1_2048_SHA512;
  }
  if (AlgStr == "RSA_PKCS1_3072_SHA384"sv) {
    return Signatures::Algorithm::RSA_PKCS1_3072_SHA384;
  }
  if (AlgStr == "RSA_PKCS1_3072_SHA512"sv) {
    return Signatures::Algorithm::RSA_PKCS1_3072_SHA512;
  }
  if (AlgStr == "RSA_PKCS1_4096_SHA512"sv) {
    return Signatures::Algorithm::RSA_PKCS1_4096_SHA512;
  }
  if (AlgStr == "RSA_PSS_2048_SHA256"sv) {
    return Signatures::Algorithm::RSA_PSS_2048_SHA256;
  }
  if (AlgStr == "RSA_PSS_2048_SHA384"sv) {
    return Signatures::Algorithm::RSA_PSS_2048_SHA384;
  }
  if (AlgStr == "RSA_PSS_2048_SHA512"sv) {
    return Signatures::Algorithm::RSA_PSS_2048_SHA512;
  }
  if (AlgStr == "RSA_PSS_3072_SHA384"sv) {
    return Signatures::Algorithm::RSA_PSS_3072_SHA384;
  }
  if (AlgStr == "RSA_PSS_3072_SHA512"sv) {
    return Signatures::Algorithm::RSA_PSS_3072_SHA512;
  }
  if (AlgStr == "RSA_PSS_4096_SHA512"sv) {
    return Signatures::Algorithm::RSA_PSS_4096_SHA512;
  }
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
}
/// -----------------------------------------------------------------------------

/// assuming exist or return `_algorithm_failure`
#define assumingExist(Expr)                                                    \
  do {                                                                         \
    if (unlikely(!(Expr))) {                                                   \
      return __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE;                            \
    }                                                                          \
  } while (0)

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge