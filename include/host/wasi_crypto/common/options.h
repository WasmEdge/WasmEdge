// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "wasi_crypto/api.hpp"

#include <memory>
#include <string_view>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

// For future will extend it.
class OptionBase {
public:
  virtual ~OptionBase() = default;

  virtual WasiCryptoExpect<void> set(std::string_view Name,
                                     Span<uint8_t const> Value);

  virtual WasiCryptoExpect<void> setU64(std::string_view Name, uint64_t Value);

  virtual WasiCryptoExpect<void> setGuestBuffer(std::string_view Name,
                                                Span<uint8_t> Buffer);

  virtual WasiCryptoExpect<std::vector<uint8_t>> get(std::string_view Name);

  virtual WasiCryptoExpect<uint64_t> getU64(std::string_view Name);

  static WasiCryptoExpect<std::unique_ptr<OptionBase>> make(__wasi_algorithm_type_e_t Algorithm);

};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge