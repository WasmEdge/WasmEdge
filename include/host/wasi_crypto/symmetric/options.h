// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/lock.h"
#include "wasi_crypto/api.hpp"

#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

class Option {
public:
  WasiCryptoExpect<void> set(std::string_view Name, Span<const uint8_t> Value);

  WasiCryptoExpect<void> setU64(std::string_view Name, uint64_t Value);

  WasiCryptoExpect<void> setGuestBuffer(std::string_view Name,
                                        Span<uint8_t> Buffer);

  WasiCryptoExpect<std::vector<uint8_t>> get(std::string_view Name) const;

  WasiCryptoExpect<uint64_t> getU64(std::string_view Name) const;

private:
  mutable std::shared_mutex Mutex;

  std::optional<std::vector<uint8_t>> Context;
  std::optional<std::vector<uint8_t>> Salt;
  std::optional<std::vector<uint8_t>> Nonce;
  std::optional<uint64_t> MemoryLimit;
  std::optional<uint64_t> OpsLimit;
  std::optional<uint64_t> Parallelism;
  std::optional<Span<uint8_t>> GuestBuffer;
};

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge