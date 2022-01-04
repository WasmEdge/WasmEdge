// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
class CryptoRandom {
public:
  WasiCryptoExpect<void> fill(Span<uint8_t> Bytes);
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
