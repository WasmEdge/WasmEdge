// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"

#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class KxPublickey {};

class KxPublickeyBuilder {
public:
  virtual ~KxPublickeyBuilder() = default;

  virtual WasiCryptoExpect<KxPublickey> fromRaw(Span<uint8_t const> Raw) = 0;

  static WasiCryptoExpect<std::unique_ptr<KxPublickeyBuilder>>
  builder(std::string_view Alg);
};

class KxPublicKey {
public:
  virtual ~KxPublicKey() = default;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
