// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/session/session_state.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

class XoodyakKeyBuilder : public Key::Builder {
public:
  using Builder::Builder;

  WasiCryptoExpect<std::unique_ptr<Key>>
  generate(std::shared_ptr<Option> OptOption) override;

  WasiCryptoExpect<std::unique_ptr<Key>>
  import(Span<uint8_t const> Raw) override;

  WasiCryptoExpect<__wasi_size_t> keyLen() override;

private:
  SymmetricAlgorithm Alg;
};

class XoodyakState : public SessionState {
public:
  static WasiCryptoExpect<std::unique_ptr<XoodyakState>>
  open(SymmetricAlgorithm Alg, std::shared_ptr<Key> OptKey,
       std::shared_ptr<Option> OptOption);
  WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override;
  WasiCryptoExpect<__wasi_size_t> maxTagLen() override;
  WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) override;
  WasiCryptoExpect<__wasi_size_t>
  decryptDetached(Span<uint8_t> Out, Span<const uint8_t> Data,
                  Span<uint8_t> RawTag) override;
};

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
