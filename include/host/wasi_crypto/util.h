// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

template <auto fn> using Deleter = std::integral_constant<decltype(fn), fn>;

template <typename T, auto fn>
using OpenSSlUniquePtr = std::unique_ptr<T, Deleter<fn>>;

template <typename T, typename U> std::optional<T> parseCUnion(U Union);

template <>
inline std::optional<__wasi_options_t> parseCUnion(__wasi_opt_options_t Union) {
  if (Union.tag == __WASI_OPT_OPTIONS_U_SOME) {
    return Union.u.some;
  }
  return std::nullopt;
}

template <>
inline std::optional<__wasi_symmetric_key_t>
parseCUnion(__wasi_opt_symmetric_key_t Union) {
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

template <typename T> WASICrypto::WasiCryptoExpect<T> cast(uint64_t) noexcept;

template <>
WASICrypto::WasiCryptoExpect<__wasi_algorithm_type_e_t>
cast(uint64_t Algorithm) noexcept {
  switch (static_cast<WasiRawTypeT<__wasi_algorithm_type_e_t>>(Algorithm)) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES:
  case __WASI_ALGORITHM_TYPE_SYMMETRIC:
  case __WASI_ALGORITHM_TYPE_KEY_EXCHANGE:
    return static_cast<__wasi_algorithm_type_e_t>(Algorithm);
  default:
    return WASICrypto::WasiCryptoUnexpect(
        __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
