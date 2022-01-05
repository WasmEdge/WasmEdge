// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/common/options.h"
#include "host/wasi_crypto/lock.h"
#include "host/wasi_crypto/symmetric/alg.h"
#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/options.h"
#include "host/wasi_crypto/symmetric/tag.h"
#include "wasi_crypto/api.hpp"

#include <memory>
#include <mutex>
#include <optional>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

/// Each algorithm type supports a subset of symmetric_state_*() functions.
///
/// If a function is not defined for an algorithm, it MUST unconditionally
/// return an unsupported_feature error code.
class State {
public:
  virtual WasiCryptoExpect<std::vector<uint8_t>>
  optionsGet(std::string_view Name) = 0;

  virtual WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) = 0;

  virtual WasiCryptoExpect<void> absorb(Span<uint8_t const> Data) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  /// The output buffer given to the squeeze function can be smaller than the
  /// hash function output size. In that case, the implementation MUST
  /// truncate the output to the requested length.
  ///
  /// If the requested size exceeds what the hash function can output, the
  /// invalid_length error code MUST be returned.
  virtual WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  virtual WasiCryptoExpect<void> ratchet() {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  virtual WasiCryptoExpect<__wasi_size_t> encrypt(Span<uint8_t> Out,
                                                  Span<uint8_t const> Data) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }


  virtual WasiCryptoExpect<Tag> encryptDetached(Span<uint8_t> Out,
                                                Span<uint8_t const> Data) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  virtual WasiCryptoExpect<__wasi_size_t> decrypt(Span<uint8_t> Out,
                                                  Span<uint8_t const> Data) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  virtual WasiCryptoExpect<__wasi_size_t>
  decryptDetached(Span<uint8_t> Out, Span<uint8_t const> Data,
                  Span<uint8_t> RawTag) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  virtual WasiCryptoExpect<Key> squeezeKey(SymmetricAlgorithm KeyAlg) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  virtual WasiCryptoExpect<Tag> squeezeTag() {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  virtual WasiCryptoExpect<__wasi_size_t> maxTagLen() {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  std::shared_mutex Mutex;
};

template <typename T>
constexpr std::unique_ptr<T> builder(SymmetricAlgorithm Alg);

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
