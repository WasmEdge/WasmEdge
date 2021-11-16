// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/symmetric/alg.h"
#include "host/wasi_crypto/symmetric/options.h"

#include <memory>
#include <mutex>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class SymmetricKey;

class SymmetricKeyBuilder {
public:
  virtual ~SymmetricKeyBuilder() = default;

  virtual WasiCryptoExpect<SymmetricKey>
  generate(std::optional<SymmetricOptions> Option) = 0;

  virtual WasiCryptoExpect<SymmetricKey> import(Span<uint8_t const> Raw) = 0;

  virtual WasiCryptoExpect<__wasi_size_t> keyLen() = 0;
};

class SymmetricKeyBase {
public:
  virtual ~SymmetricKeyBase() = default;
  // lock
  virtual WasiCryptoExpect<std::vector<uint8_t>> raw() = 0;

  virtual SymmetricAlgorithm alg() = 0;
};

class SymmetricKey {
public:
  SymmetricKey(std::unique_ptr<SymmetricKeyBase> Inner);

  static WasiCryptoExpect<SymmetricKey>
  generate(SymmetricAlgorithm Alg, std::optional<SymmetricOptions> OptOption);

  static WasiCryptoExpect<SymmetricKey> import(SymmetricAlgorithm Alg,
                                               Span<uint8_t const> Raw);

  WasiCryptoExpect<std::vector<uint8_t>> raw() {
    return Inner->locked(
        [](std::unique_ptr<SymmetricKeyBase> &Data) { return Data->raw(); });
  }

  SymmetricAlgorithm alg() {
    return Inner->locked(
        [](std::unique_ptr<SymmetricKeyBase> &Data) { return Data->alg(); });
  }

  template <typename T, std::enable_if_t<std::is_base_of_v<SymmetricKeyBase, T>,
                                         bool> = true>
  WasiCryptoExpect<void> isType() {
    return Inner->template locked(
        [](std::unique_ptr<SymmetricKeyBase> &Data) -> WasiCryptoExpect<void> {
          if (dynamic_cast<T*>(Data.get()) == nullptr) {
            return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
          }
          return {};
        });
  };

private:
  static WasiCryptoExpect<std::unique_ptr<SymmetricKeyBuilder>>
  builder(SymmetricAlgorithm Alg);

  std::shared_ptr<Mutex<std::unique_ptr<SymmetricKeyBase>>> Inner;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
