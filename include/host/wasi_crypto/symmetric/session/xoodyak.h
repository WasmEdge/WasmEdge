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
  XoodyakKeyBuilder(SymmetricAlgorithm Alg);

  WasiCryptoExpect<Key> generate(std::shared_ptr<Options> OptOption) override;

  WasiCryptoExpect<Key> import(Span<uint8_t const> Raw) override;

  WasiCryptoExpect<__wasi_size_t> keyLen() override;

private:
  SymmetricAlgorithm Alg;
};

class XoodyakState : public SessionState {
public:
  static WasiCryptoExpect<std::unique_ptr<XoodyakState>>
  import(SymmetricAlgorithm Alg, std::optional<Key> OptKey,
         std::shared_ptr<Options> OptOptions);
};

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
