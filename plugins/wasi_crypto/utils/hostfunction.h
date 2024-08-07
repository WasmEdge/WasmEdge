// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/hostfunc.h - HostFunction class ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the HostFunction classes and some helper functions
/// interact with wasi.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "ctx.h"
#include "symmetric/registered.h"
#include "utils/error.h"

#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

/// The wasi-crypto's base HostFunction class, every wasi-crypto HostFunction
/// should inherit from this class.
template <typename T> class HostFunction : public Runtime::HostFunction<T> {
public:
  HostFunction(Context &Ctx) : Runtime::HostFunction<T>(0), Ctx(Ctx) {}

protected:
  Context &Ctx;
};

/// Cast wasi enum to c++ enum.
template <typename T> constexpr WasiCryptoExpect<T> cast(uint64_t) noexcept;

template <typename T> struct WasiRawType {
  using Type = std::underlying_type_t<T>;
};
template <> struct WasiRawType<uint8_t> {
  using Type = uint8_t;
};
template <> struct WasiRawType<uint16_t> {
  using Type = uint16_t;
};
template <> struct WasiRawType<uint32_t> {
  using Type = uint32_t;
};
template <> struct WasiRawType<uint64_t> {
  using Type = uint64_t;
};

template <typename T> using WasiRawTypeT = typename WasiRawType<T>::Type;

template <>
constexpr WasiCryptoExpect<__wasi_algorithm_type_e_t>
cast(uint64_t AlgType) noexcept {
  switch (static_cast<WasiRawTypeT<__wasi_algorithm_type_e_t>>(AlgType)) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES:
  case __WASI_ALGORITHM_TYPE_SYMMETRIC:
  case __WASI_ALGORITHM_TYPE_KEY_EXCHANGE:
    return static_cast<__wasi_algorithm_type_e_t>(AlgType);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
  }
}

template <>
constexpr WasiCryptoExpect<__wasi_keypair_encoding_e_t>
cast(uint64_t Encoding) noexcept {
  switch (static_cast<WasiRawTypeT<__wasi_keypair_encoding_e_t>>(Encoding)) {
  case __WASI_KEYPAIR_ENCODING_RAW:
  case __WASI_KEYPAIR_ENCODING_PKCS8:
  case __WASI_KEYPAIR_ENCODING_PEM:
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

/// Cast c++ size_t to wasi size_t.
constexpr WasiCryptoExpect<__wasi_size_t> toWasiSize(size_t Size) noexcept {
  ensureOrReturn(Size <= std::numeric_limits<__wasi_size_t>::max(),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  return static_cast<__wasi_size_t>(Size);
}

/// Convert string_view to inner Alg representation.
template <typename T> WasiCryptoExpect<T> tryFrom(std::string_view) noexcept;

template <>
WasiCryptoExpect<Symmetric::Algorithm>
tryFrom(std::string_view RawAlgStr) noexcept;

WasiCryptoExpect<AsymmetricCommon::Algorithm>
tryFrom(__wasi_algorithm_type_e_t AlgType, std::string_view RawAlgStr) noexcept;

template <>
WasiCryptoExpect<Kx::Algorithm> tryFrom(std::string_view RawAlgStr) noexcept;

template <>
WasiCryptoExpect<Signatures::Algorithm>
tryFrom(std::string_view RawAlgStr) noexcept;

/// Check exist or return `_algorithm_failure`.
#define checkExist(Expr)                                                       \
  do {                                                                         \
    if (unlikely(!(Expr))) {                                                   \
      return __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE;                            \
    }                                                                          \
  } while (0)

/// Check Span exist or return `_algorithm_failure`.
#define checkRangeExist(Expr, Size)                                            \
  do {                                                                         \
    if (unlikely((Expr).size() != (Size))) {                                   \
      return __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE;                            \
    }                                                                          \
  } while (0)

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
