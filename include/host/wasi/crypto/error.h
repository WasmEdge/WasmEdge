// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/expected.h"
#include "wasi/crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WASI {
namespace Crypto {
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

} // namespace Crypto
} // namespace WASI
} // namespace Host
} // namespace WasmEdge