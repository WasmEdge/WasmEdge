// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/common/options.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Common {

Options optionsOpen(__wasi_algorithm_type_e_t Alg) noexcept {
  switch (Alg) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES:
    return Signatures::Options{};
  case __WASI_ALGORITHM_TYPE_SYMMETRIC:
    return Symmetric::Options{};
  case __WASI_ALGORITHM_TYPE_KEY_EXCHANGE:
    return Kx::Options{};
  default:
    assumingUnreachable();
  }
}

WasiCryptoExpect<void> optionsSet(Options &Options, std::string_view Name,
                                  Span<const uint8_t> Value) noexcept {
  return std::visit(
      [Name, Value](auto &&Option) { return Option.set(Name, Value); },
      Options);
}

WasiCryptoExpect<void> optionsSetU64(Options &Options, std::string_view Name,
                                     uint64_t Value) noexcept {
  return std::visit(
      [Name, Value](auto &&Option) { return Option.setU64(Name, Value); },
      Options);
}

WasiCryptoExpect<void> optionsSetGuestBuffer(Options &Options,
                                             std::string_view Name,
                                             Span<uint8_t> Value) noexcept {
  return std::visit(
      [Name, Value](auto &&Option) {
        return Option.setGuestBuffer(Name, Value);
      },
      Options);
}

} // namespace Common
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge