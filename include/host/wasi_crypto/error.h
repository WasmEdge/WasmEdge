// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/expected.h"
#include "wasi_crypto/api.hpp"

#include <cassert>
#include <cstddef>
#ifdef NDEBUG
#define assumingUnreachable() __builtin_unreachable()
#else
#define assumingUnreachable()                                                  \
  (assert(false && "unreachable"), __builtin_unreachable())
#endif

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
/// Type aliasing for Expected<T, __wasi_crypto_errno_t>.
template <typename T>
using WasiCryptoExpect = Expected<T, __wasi_crypto_errno_e_t>;

/// Helper function for Unexpected<ErrCode>.
constexpr auto WasiCryptoUnexpect(__wasi_crypto_errno_e_t Val) {
  return Unexpected<__wasi_crypto_errno_e_t>(Val);
}
template <typename T>
constexpr auto WasiCryptoUnexpect(const WasiCryptoExpect<T> &Val) {
  return Unexpected<__wasi_crypto_errno_e_t>(Val.error());
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge