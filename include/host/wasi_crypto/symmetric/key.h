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

class SymmetricKey {
public:
  class Base {
  public:
    virtual ~Base() = default;
    // lock
    virtual Span<const uint8_t> asRef() = 0;

    virtual SymmetricAlgorithm alg() = 0;
  };

  class Builder {
  public:
    virtual ~Builder() = default;

    virtual WasiCryptoExpect<SymmetricKey>
    generate(std::optional<SymmetricOptions> Option) = 0;

    virtual WasiCryptoExpect<SymmetricKey> import(Span<uint8_t const> Raw) = 0;

    virtual WasiCryptoExpect<__wasi_size_t> keyLen() = 0;
  };

  SymmetricKey(std::unique_ptr<Base> Inner)
      : Inner(
            std::make_shared<Mutex<std::unique_ptr<Base>>>(std::move(Inner))) {}

  static WasiCryptoExpect<SymmetricKey>
  generate(SymmetricAlgorithm Alg, std::optional<SymmetricOptions> OptOption);

  static WasiCryptoExpect<SymmetricKey> import(SymmetricAlgorithm Alg,
                                               Span<uint8_t const> Raw);

  static WasiCryptoExpect<SymmetricKey> from(SymmetricAlgorithm Alg,
                                             std::vector<uint8_t>&& Data);

  WasiCryptoExpect<std::vector<uint8_t>> raw() {
    return Inner->locked([](std::unique_ptr<Base> &Data)
                             -> WasiCryptoExpect<std::vector<uint8_t>> {
      auto Ref = Data->asRef();
      return std::vector<uint8_t>{Ref.begin(), Ref.end()};
    });
  }

  auto &inner() { return Inner; }

  template <typename T,
            std::enable_if_t<std::is_base_of_v<Base, T>, bool> = true>
  WasiCryptoExpect<void> isType() {
    return Inner->template locked(
        [](std::unique_ptr<Base> &Data) -> WasiCryptoExpect<void> {
          if (dynamic_cast<T *>(Data.get()) == nullptr) {
            return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
          }
          return {};
        });
  };

private:
  static WasiCryptoExpect<std::unique_ptr<Builder>>
  builder(SymmetricAlgorithm Alg);

  std::shared_ptr<Mutex<std::unique_ptr<Base>>> Inner;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
