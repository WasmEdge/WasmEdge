// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/lock.h"
#include "wasi_crypto/api.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

struct SymmetricOptionsInner {
  std::optional<std::vector<uint8_t>> Context;
  std::optional<std::vector<uint8_t>> Salt;
  std::optional<std::vector<uint8_t>> Nonce;
  std::optional<uint64_t> MemoryLimit;
  std::optional<uint64_t> OpsLimit;
  std::optional<uint64_t> Parallelism;
  std::optional<Span<uint8_t>> GuestBuffer;
};

class SymmetricOptions {
public:
  WasiCryptoExpect<void> set(std::string_view Name, Span<const uint8_t> Value);

  WasiCryptoExpect<void> setU64(std::string_view Name, uint64_t Value);

  WasiCryptoExpect<void> setGuestBuffer(std::string_view Name,
                                        Span<uint8_t> GuestBuffer);

  WasiCryptoExpect<std::vector<uint8_t>> get(std::string_view Name);

  WasiCryptoExpect<uint64_t> getU64(std::string_view Name);

private:
  std::shared_ptr<Mutex<SymmetricOptionsInner>> Inner =
      std::make_shared<Mutex<SymmetricOptionsInner>>(SymmetricOptionsInner{});
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge