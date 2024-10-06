// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/options.h"

#include <utility>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Common {

Options optionsOpen(__wasi_algorithm_type_e_t Alg) noexcept {
  switch (Alg) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES:
    return Options{std::in_place_type<Signatures::Options>};
  case __WASI_ALGORITHM_TYPE_SYMMETRIC:
    return Options{std::in_place_type<Symmetric::Options>};
  case __WASI_ALGORITHM_TYPE_KEY_EXCHANGE:
    return Options{std::in_place_type<Kx::Options>};
  default:
    assumingUnreachable();
  }
}

WasiCryptoExpect<void> optionsSet(Options &Options, std::string_view Name,
                                  Span<const uint8_t> Value) noexcept {
  return std::visit(
      [Name, Value](auto &Option) noexcept { return Option.set(Name, Value); },
      Options);
}

WasiCryptoExpect<void> optionsSetU64(Options &Options, std::string_view Name,
                                     uint64_t Value) noexcept {
  return std::visit(
      [Name, Value](auto &Option) noexcept {
        return Option.setU64(Name, Value);
      },
      Options);
}

WasiCryptoExpect<void> optionsSetGuestBuffer(Options &Options,
                                             std::string_view Name,
                                             Span<uint8_t> Value) noexcept {
  return std::visit(
      [Name, Value](auto &Option) noexcept {
        return Option.setGuestBuffer(Name, Value);
      },
      Options);
}

} // namespace Common
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
