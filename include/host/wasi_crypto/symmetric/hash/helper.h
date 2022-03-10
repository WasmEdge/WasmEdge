// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/symmetric/hash/helper.h - Hash helper relative --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration hash function helper.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/symmetric/alg.h"
#include "host/wasi_crypto/symmetric/options.h"
#include "host/wasi_crypto/symmetric/tag.h"
#include "host/wasi_crypto/utils/error.h"
#include "host/wasi_crypto/utils/optional.h"

#include <optional>
#include <string_view>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

/// Hash never have key, just a placement, every hash key should inherent from
/// this class
///
/// More detailed:
/// https://github.com/WebAssembly/wasi-crypto/blob/main/docs/wasi-crypto.md#hash-functions
template <typename Key> class HashKey {
public:
  static WasiCryptoExpect<Key> import(Span<const uint8_t>) noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_KEY_NOT_SUPPORTED);
  }

  static WasiCryptoExpect<Key> generate(OptionalRef<Options>) noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_KEY_NOT_SUPPORTED);
  }

  std::vector<uint8_t> exportData() const noexcept { assumingUnreachable(); }
};

/// Hash invalid operation, every hash state should inherent from this class
template <typename Key> class HashState {
public:
  /// current hash not support any options
  WasiCryptoExpect<size_t> optionsGet(std::string_view,
                                      Span<uint8_t>) const noexcept {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
  }

  /// current hash not support any options
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

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
