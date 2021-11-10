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

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
