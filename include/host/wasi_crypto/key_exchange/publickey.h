// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"

#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class KxPublicKey {
public:
  virtual ~KxPublicKey() = default;
};

class KxPublicKeyBuilder {
public:
  virtual ~KxPublicKeyBuilder() = default;

  virtual WasiCryptoExpect<KxPublicKey> fromRaw(Span<uint8_t const> Raw) = 0;

  static WasiCryptoExpect<std::unique_ptr<KxPublicKeyBuilder>>
  builder(std::string_view Alg);
};



} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
