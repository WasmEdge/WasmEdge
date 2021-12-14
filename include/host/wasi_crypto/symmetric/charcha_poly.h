// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/expected.h"
#include "experimental/expected.hpp"
#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/state.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class ChaChaPolySymmetricKey : public SymmetricKey::Base {
public:
  ChaChaPolySymmetricKey(SymmetricAlgorithm Alg, Span<uint8_t const> Raw);

  Span<const uint8_t> asRef() override { return Raw; }

  SymmetricAlgorithm alg() override { return Alg;}

private:
  SymmetricAlgorithm Alg;
  std::vector<uint8_t> Raw;
};

class ChaChaPolySymmetricKeyBuilder : public SymmetricKey::Builder {
public:
  ChaChaPolySymmetricKeyBuilder(SymmetricAlgorithm Alg);

  WasiCryptoExpect<SymmetricKey>
  generate(std::optional<SymmetricOptions> OptOption) override;

  WasiCryptoExpect<SymmetricKey> import(Span<uint8_t const> Raw) override;

  WasiCryptoExpect<__wasi_size_t> keyLen() override;

private:
  SymmetricAlgorithm Alg;
};

class ChaChaPolySymmetricState : public SymmetricState::Base {
public:
  static WasiCryptoExpect<std::unique_ptr<ChaChaPolySymmetricState>>
  make(SymmetricAlgorithm Alg, std::optional<SymmetricKey> OptKey,
       std::optional<SymmetricOptions> OptOptions);
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
