// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi/crypto/error.h"
#include "wasi/crypto/api.hpp"

#include <string_view>

namespace WasmEdge {
namespace Host {
namespace WASI {
namespace Crypto {

// For future will extend it.
class Options {
public:
  virtual ~Options() = default;

  virtual WasiCryptoExpect<void> set(std::string_view Name,
                                     Span<uint8_t const> Value);

  virtual WasiCryptoExpect<void> setU64(std::string_view Name, uint64_t Value);

  virtual WasiCryptoExpect<void> setGuestBuffer(std::string_view Name,
                                                Span<uint8_t> Buffer);

  virtual WasiCryptoExpect<std::vector<uint8_t>> get(std::string_view Name);

  virtual WasiCryptoExpect<uint64_t> getU64(std::string_view Name);
};

} // namespace Crypto
} // namespace WASI
} // namespace Host
} // namespace WasmEdge