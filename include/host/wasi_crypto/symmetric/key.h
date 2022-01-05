// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/symmetric/alg.h"
#include "host/wasi_crypto/symmetric/options.h"

#include <memory>
#include <mutex>
#include <shared_mutex>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

/// Keys are represented by the symmetric_key type for all symmetric operations,
/// and are tied to an algorithm. They can only be serialized as raw bytes,
/// therefore, the import and export functions omit an encoding parameter.
class Key {
public:
  auto alg() { return Alg; }
  const auto &raw() { return Data; }

  class Builder {
  public:
    Builder() = default;
    virtual ~Builder() = default;

    /// Generate a key by specific algorithm and option.
    ///
    /// @param[in] Option Opt option
    /// @return a symmetric key on success.
    /// an `__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM` error code if the
    /// algorithm is not supported by the host
    virtual WasiCryptoExpect<Key>
    generate(std::shared_ptr<Options> OptOption) = 0;

    /// Import key from raw bytes.
    ///
    /// @param[in] Raw Input key
    /// @return a symmetric key on success.
    /// an `__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM` error code if the
    /// algorithm is not supported by the host
    /// an `__WASI_CRYPTO_ERRNO_INVALID_KEY` error code if the key size is
    /// invalid for the given algorithm
    virtual WasiCryptoExpect<Key> import(Span<uint8_t const> Raw) = 0;

    virtual WasiCryptoExpect<__wasi_size_t> keyLen() = 0;

  private:
    SymmetricAlgorithm Alg;
  };

  static WasiCryptoExpect<Key> generate(SymmetricAlgorithm Alg,
                                        std::shared_ptr<Options> OptOption);

  static WasiCryptoExpect<Key> import(SymmetricAlgorithm Alg,
                                      Span<uint8_t const> Raw);

private:
  std::shared_mutex Mutex;

  SymmetricAlgorithm Alg;
  std::vector<uint8_t> Data;
};

using OptKeyPtr = std::shared_ptr<Key>;

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
