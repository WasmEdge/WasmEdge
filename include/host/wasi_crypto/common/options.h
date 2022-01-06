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
namespace Common {

class Options {
public:
  Options(__wasi_algorithm_type_e_t /*Alg*/) /*: Alg(Alg)*/ {}

  virtual ~Options() = default;

  static std::unique_ptr<Options> open(__wasi_algorithm_type_e_t Alg);

  virtual WasiCryptoExpect<void> set(std::string_view Name,
                                     Span<const uint8_t> Value);

  virtual WasiCryptoExpect<void> setU64(std::string_view Name, uint64_t Value);

  virtual WasiCryptoExpect<void> setGuestBuffer(std::string_view Name,
                                                Span<uint8_t> Buffer);

  virtual WasiCryptoExpect<std::vector<uint8_t>>
  get(std::string_view Name) const;

  virtual WasiCryptoExpect<uint64_t> getU64(std::string_view Name) const;

private:
//  __wasi_algorithm_type_e_t Alg;
};

} // namespace Common
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge