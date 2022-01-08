// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/error.h"
#include "openssl/err.h"
#include "wasi_crypto/api.hpp"

#include <memory>
#include <optional>
#include <string_view>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

template <typename T, typename U>
constexpr std::optional<T> parseCUnion(U Union) noexcept;

template <>
constexpr std::optional<__wasi_options_t>
parseCUnion(__wasi_opt_options_t Union) noexcept {
  if (Union.tag == __WASI_OPT_OPTIONS_U_SOME) {
    return Union.u.some;
  }
  return std::nullopt;
}

template <>
constexpr std::optional<__wasi_symmetric_key_t>
parseCUnion(__wasi_opt_symmetric_key_t Union) noexcept {
  if (Union.tag == __WASI_OPT_SYMMETRIC_KEY_U_SOME) {
    return Union.u.some;
  }
  return std::nullopt;
}

template <typename T> struct WasiRawType {
  using Type = std::underlying_type_t<T>;
};
template <> struct WasiRawType<uint8_t> { using Type = uint8_t; };
template <> struct WasiRawType<uint16_t> { using Type = uint16_t; };
template <> struct WasiRawType<uint32_t> { using Type = uint32_t; };
template <> struct WasiRawType<uint64_t> { using Type = uint64_t; };

template <typename T> using WasiRawTypeT = typename WasiRawType<T>::Type;

template <typename T> constexpr WasiCryptoExpect<T> cast(uint64_t) noexcept;

template <>
constexpr WasiCryptoExpect<__wasi_algorithm_type_e_t>
cast(uint64_t Algorithm) noexcept {
  switch (static_cast<WasiRawTypeT<__wasi_algorithm_type_e_t>>(Algorithm)) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES:
  case __WASI_ALGORITHM_TYPE_SYMMETRIC:
  case __WASI_ALGORITHM_TYPE_KEY_EXCHANGE:
    return static_cast<__wasi_algorithm_type_e_t>(Algorithm);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
}

template <>
constexpr WasiCryptoExpect<__wasi_keypair_encoding_e_t>
cast(uint64_t Algorithm) noexcept {
  switch (static_cast<WasiRawTypeT<__wasi_keypair_encoding_e_t>>(Algorithm)) {
  case __WASI_KEYPAIR_ENCODING_RAW:
  case __WASI_KEYPAIR_ENCODING_PKCS8:
  case __WASI_KEYPAIR_ENCODING_PEM:
  case __WASI_KEYPAIR_ENCODING_LOCAL:
    return static_cast<__wasi_keypair_encoding_e_t>(Algorithm);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
}

template <>
constexpr WasiCryptoExpect<__wasi_publickey_encoding_e_t>
cast(uint64_t Algorithm) noexcept {
  switch (static_cast<WasiRawTypeT<__wasi_publickey_encoding_e_t>>(Algorithm)) {
  case __WASI_PUBLICKEY_ENCODING_RAW:
  case __WASI_PUBLICKEY_ENCODING_PKCS8:
  case __WASI_PUBLICKEY_ENCODING_PEM:
  case __WASI_PUBLICKEY_ENCODING_SEC:
  case __WASI_PUBLICKEY_ENCODING_COMPRESSED_SEC:
  case __WASI_PUBLICKEY_ENCODING_LOCAL:
    return static_cast<__wasi_publickey_encoding_e_t>(Algorithm);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
}

template <>
constexpr WasiCryptoExpect<__wasi_secretkey_encoding_e_t>
cast(uint64_t Algorithm) noexcept {
  switch (static_cast<WasiRawTypeT<__wasi_secretkey_encoding_e_t>>(Algorithm)) {
  case __WASI_SECRETKEY_ENCODING_RAW:
  case __WASI_SECRETKEY_ENCODING_PKCS8:
  case __WASI_SECRETKEY_ENCODING_PEM:
  case __WASI_SECRETKEY_ENCODING_SEC:
  case __WASI_SECRETKEY_ENCODING_COMPRESSED_SEC:
  case __WASI_SECRETKEY_ENCODING_LOCAL:
    return static_cast<__wasi_secretkey_encoding_e_t>(Algorithm);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
}

template <>
inline WasiCryptoExpect<__wasi_signature_encoding_e_t>
cast(uint64_t Algorithm) noexcept {
  switch (static_cast<WasiRawTypeT<__wasi_signature_encoding_e_t>>(Algorithm)) {
  case __WASI_SIGNATURE_ENCODING_RAW:
  case __WASI_SIGNATURE_ENCODING_DER:
    return static_cast<__wasi_signature_encoding_e_t>(Algorithm);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
}

template <typename T>
constexpr WasiCryptoExpect<T> tryFrom(std::string_view) noexcept;

#ifdef NDEBUG
#define opensslAssuming(Cond)                                                  \
  (static_cast<bool>(Cond) ? static_cast<void>(0) : __builtin_unreachable())
#else
#define opensslAssuming(Cond)                                                  \
  (static_cast<bool>(Cond)                                                     \
       ? static_cast<void>(0)                                                  \
       : (ERR_print_errors_fp(stderr),                                         \
          OPENSSL_die("assertion failed: " #Cond, __FILE__, __LINE__)))
#endif

template <auto Fn> using Deleter = std::integral_constant<decltype(Fn), Fn>;

template <typename T, auto Fn>
using OpenSSLUniquePtr = std::unique_ptr<T, Deleter<Fn>>;

/// ensure Expr is true or return ErrorCode
#define ensureOrReturn(Expr, ErrorCode)                                        \
  do {                                                                         \
    if (!(Expr)) {                                                             \
      return WasiCryptoUnexpect((ErrorCode));                                  \
    }                                                                          \
  } while (0)


template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };

template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
