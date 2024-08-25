// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/utils/error.h - Error definition -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the wasi-crypto error handling related functions.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "common/errcode.h"
#include "common/expected.h"
#include "wasi_crypto/api.hpp"

#include <cassert>
#include <cstddef>

/// Ensure the Expr is true or return ErrorCode.
#define ensureOrReturn(Expr, ErrorCode)                                        \
  do {                                                                         \
    if (!(Expr)) {                                                             \
      return WasiCryptoUnexpect((ErrorCode));                                  \
    }                                                                          \
  } while (0)

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

/// Type aliasing for Expected<T, __wasi_crypto_errno_t>.
template <typename T>
using WasiCryptoExpect = Expected<T, __wasi_crypto_errno_e_t>;

/// Helper function for Unexpected<ErrCode>.
constexpr auto WasiCryptoUnexpect(__wasi_crypto_errno_e_t Val) noexcept {
  return Unexpected<__wasi_crypto_errno_e_t>(Val);
}
template <typename T>
constexpr auto WasiCryptoUnexpect(const WasiCryptoExpect<T> &Val) noexcept {
  return Unexpected<__wasi_crypto_errno_e_t>(Val.error());
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
