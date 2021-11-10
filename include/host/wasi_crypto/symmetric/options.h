// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/common/options.h"
#include "host/wasi_crypto/error.h"
#include "wasi_crypto/api.hpp"
#include "host/wasi_crypto/lock.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
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
  // temporary
  std::optional<Span<uint8_t>> GuestBuffer;
};

class SymmetricOption : public OptionBase {
public:
  SymmetricOption();

  WasiCryptoExpect<void> set(std::string_view Name,
                             Span<const uint8_t> Value) override;

  WasiCryptoExpect<void> setU64(std::string_view Name, uint64_t Value) override;

  WasiCryptoExpect<void> setGuestBuffer(std::string_view Name,
                                        Span<uint8_t> GuestBuffer) override;

  WasiCryptoExpect<std::vector<uint8_t>> get(std::string_view Name) override;

  WasiCryptoExpect<uint64_t> getU64(std::string_view Name) override;

  Mutex<SymmetricOptionsInner> Inner;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge