// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/symmetric/algorithm.h"
#include "host/wasi_crypto/symmetric/options.h"

#include <memory>
#include <mutex>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class SymmetricKeyBuilder;

class SymmetricKey {
public:
  virtual ~SymmetricKey() = default;
  // lock
  virtual WasiCryptoExpect<Span<uint8_t>> raw() = 0;

  virtual SymmetricAlgorithm alg() = 0;

  static WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
  generate(SymmetricAlgorithm Alg, std::shared_ptr<SymmetricOption> Option);

  static WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
  import(SymmetricAlgorithm Alg, Span<uint8_t const> Raw);

private:
  static WasiCryptoExpect<std::unique_ptr<SymmetricKeyBuilder>>
  builder(SymmetricAlgorithm Alg);
};

class SymmetricKeyBuilder {
public:
  virtual ~SymmetricKeyBuilder() = default;

  virtual WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
  generate(std::shared_ptr<SymmetricOption> Option) = 0;

  virtual WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
  import(Span<uint8_t const> Raw) = 0;

  virtual WasiCryptoExpect<__wasi_size_t> keyLen() = 0;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
