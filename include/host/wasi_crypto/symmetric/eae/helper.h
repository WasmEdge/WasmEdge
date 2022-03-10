// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/symmetric/eae/helper.h - Eae helper relative ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration Extract and expand function helper.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/symmetric/alg.h"
#include "host/wasi_crypto/symmetric/tag.h"
#include "host/wasi_crypto/utils/error.h"
#include "wasi_crypto/api.hpp"

#include <optional>
#include <string_view>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

/// Expand invalid operation, every expand state should inherent from this class
///
/// More detailed:
/// https://github.com/WebAssembly/wasi-crypto/blob/main/docs/wasi-crypto.md#key-derivation-using-extract-and-expand
template <typename Key> class ExpandState {
public:
  /// current eae not support any options
  WasiCryptoExpect<size_t> optionsGet(std::string_view,
                                      Span<uint8_t>) const noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  /// current eae not support any options
  WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view) const noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  WasiCryptoExpect<void> ratchet() noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<size_t> encrypt(Span<uint8_t>,
                                   Span<const uint8_t>) noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<Tag> encryptDetached(Span<uint8_t>,
                                        Span<const uint8_t>) noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<size_t> decrypt(Span<uint8_t>,
                                   Span<const uint8_t>) noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<size_t> decryptDetached(Span<uint8_t>, Span<const uint8_t>,
                                           Span<uint8_t>) noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<Key> squeezeKey(Algorithm) noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<Tag> squeezeTag() noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<size_t> maxTagLen() const noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
};

/// Extract invalid operation, every extract state should inherent from this
/// class
template <typename Key> class ExtractState {
public:
  /// current eae not support any options
  WasiCryptoExpect<size_t> optionsGet(std::string_view,
                                      Span<uint8_t>) const noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  /// current eae not support any options
  WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view) const noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  WasiCryptoExpect<void> squeeze(Span<uint8_t>) noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<void> ratchet() noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<size_t> encrypt(Span<uint8_t>,
                                   Span<const uint8_t>) noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<Tag> encryptDetached(Span<uint8_t>,
                                        Span<const uint8_t>) noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<size_t> decrypt(Span<uint8_t>,
                                   Span<const uint8_t>) noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<size_t> decryptDetached(Span<uint8_t>, Span<const uint8_t>,
                                           Span<uint8_t>) noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<Tag> squeezeTag() noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  WasiCryptoExpect<size_t> maxTagLen() const noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
};

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
