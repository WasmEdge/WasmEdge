// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/state.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class XoodyakSymmetricKey : public SymmetricKeyBase {
public:
  XoodyakSymmetricKey(SymmetricAlgorithm Alg, Span<uint8_t const> Raw);

  WasiCryptoExpect<Span<const uint8_t>> raw() override;

  SymmetricAlgorithm alg() override;

private:
  SymmetricAlgorithm Alg;
  std::vector<uint8_t> Raw;
};

class XoodyakSymmetricKeyBuilder : public SymmetricKeyBuilder {
public:
  XoodyakSymmetricKeyBuilder(SymmetricAlgorithm Alg);

  WasiCryptoExpect<SymmetricKey>
  generate(std::optional<SymmetricOptions> OptOption) override;

  WasiCryptoExpect<SymmetricKey> import(Span<uint8_t const> Raw) override;

  WasiCryptoExpect<__wasi_size_t> keyLen() override;

private:
  SymmetricAlgorithm Alg;
};

class XoodyakSymmetricState : public SymmetricStateBase {
public:
  static WasiCryptoExpect<std::unique_ptr<XoodyakSymmetricState>>
  make(SymmetricAlgorithm Alg, std::optional<SymmetricKey> OptKey,
       std::optional<SymmetricOptions> OptOptions);
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
