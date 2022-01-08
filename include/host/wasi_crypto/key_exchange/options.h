// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include <string_view>
namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Kx {
class Options {
public:
  WasiCryptoExpect<void> set(std::string_view Name, Span<const uint8_t> Value);

  WasiCryptoExpect<void> setU64(std::string_view Name, uint64_t Value);

  WasiCryptoExpect<void> setGuestBuffer(std::string_view Name,
                                        Span<uint8_t> Buffer);

  WasiCryptoExpect<std::vector<uint8_t>> get(std::string_view Name) const;

  WasiCryptoExpect<uint64_t> getU64(std::string_view Name) const;
};

} // namespace Kx
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
