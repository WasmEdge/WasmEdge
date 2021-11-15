// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"

#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class KxSecretKey {
public:
  virtual ~KxSecretKey() = default;
};

class KxSecretKeyBuilder {
public:
  virtual ~KxSecretKeyBuilder() = default;

  virtual WasiCryptoExpect<KxSecretKey> fromRaw(Span<uint8_t const> Raw) = 0;

  static WasiCryptoExpect<std::unique_ptr<KxSecretKeyBuilder>>
  builder(std::string_view Alg);
};



} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
