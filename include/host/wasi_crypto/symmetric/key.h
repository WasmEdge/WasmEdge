// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/symmetric/alg.h"
#include "host/wasi_crypto/symmetric/options.h"

#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

/// Keys are represented by the symmetric_key type for all symmetric operations,
/// and are tied to an algorithm. They can only be serialized as raw bytes,
/// therefore, the import and export functions omit an encoding parameter.
class Key {
public:
  Key(SymmetricAlgorithm Alg, std::vector<uint8_t> &&Data)
      : Alg(Alg), Data(Data) {}

  auto alg() const { return Alg; }

  auto const &data() const { return Data; }

  class Builder {
  public:
    Builder(SymmetricAlgorithm Alg) : Alg(Alg) {}

    virtual ~Builder() = default;

    /// Generate a key by option.
    ///
    /// @param[in] OptOption Opt option
    /// @return a symmetric key on success.
    /// an `__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM` error code if the
    /// algorithm is not supported by the host
    virtual WasiCryptoExpect<std::unique_ptr<Key>>
    generate(std::shared_ptr<Options> OptOption) = 0;

    /// Import key from raw bytes.
    ///
    /// @param[in] Raw Input key
    /// @return a symmetric key on success.
    /// an `__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM` error code if the
    /// algorithm is not supported by the host
    /// an `__WASI_CRYPTO_ERRNO_INVALID_KEY` error code if the key size is
    /// invalid for the given algorithm
    virtual WasiCryptoExpect<std::unique_ptr<Key>>
    import(Span<uint8_t const> Raw) = 0;

  protected:
    const SymmetricAlgorithm Alg;
  };

  static WasiCryptoExpect<std::unique_ptr<Key>>
  generate(SymmetricAlgorithm Alg, std::shared_ptr<Options> OptOption);

  static WasiCryptoExpect<std::unique_ptr<Key>> import(SymmetricAlgorithm Alg,
                                                       Span<uint8_t const> Raw);

  static WasiCryptoExpect<std::unique_ptr<Key::Builder>>
  builder(SymmetricAlgorithm Alg);

private:
  const SymmetricAlgorithm Alg;
  const std::vector<uint8_t> Data;
};

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
