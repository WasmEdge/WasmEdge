// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/state.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class XoodyakSymmetricKey : public SymmetricKey {
public:
  XoodyakSymmetricKey(SymmetricAlgorithm Alg, Span<uint8_t const> Raw);

private:
  WasiCryptoExpect<Span<uint8_t>> raw() override;
  SymmetricAlgorithm alg() override;

private:
  SymmetricAlgorithm Alg;
  std::vector<uint8_t> Raw;
};

class XoodyakSymmetricKeyBuilder : public SymmetricKeyBuilder {
public:
  XoodyakSymmetricKeyBuilder(SymmetricAlgorithm Alg);

  WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
  generate(std::shared_ptr<SymmetricOptions> Option) override;

  WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
  import(Span<uint8_t const> Raw) override;

  WasiCryptoExpect<__wasi_size_t> keyLen() override;

private:
  SymmetricAlgorithm Alg;
};

class XoodyakSymmetricState : public SymmetricState {
public:
  static WasiCryptoExpect<std::unique_ptr<XoodyakSymmetricState>>
  make(SymmetricAlgorithm Alg, std::shared_ptr<SymmetricKey> OptKey,
       std::shared_ptr<SymmetricOptions> OptOptions);
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
