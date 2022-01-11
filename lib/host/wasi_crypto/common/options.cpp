// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/common/options.h"
#include "host/wasi_crypto/key_exchange/options.h"
#include "host/wasi_crypto/util.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Common {

Options optionsOpen(__wasi_algorithm_type_e_t Alg) {
  switch (Alg) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES:
    return std::make_unique<Signatures::Options>();
  case __WASI_ALGORITHM_TYPE_SYMMETRIC:
    return std::make_unique<Symmetric::Option>();
  case __WASI_ALGORITHM_TYPE_KEY_EXCHANGE:
    return std::make_unique<Kx::Options>();
  default:
    assumingUnreachable();
  }
}

WasiCryptoExpect<void> optionsSet(Options Options, std::string_view Name,
                                  Span<const uint8_t> Value) {
  return std::visit(
      [Name, Value](auto &&Option) -> WasiCryptoExpect<void> {
        return Option->set(Name, Value);
      },
      Options);
}

WasiCryptoExpect<void> optionsSetU64(Options Options, std::string_view Name,
                                     uint64_t Value) {
  return std::visit(
      [Name, Value](auto &&Option) -> WasiCryptoExpect<void> {
        return Option->setU64(Name, Value);
      },
      Options);
}

WasiCryptoExpect<void> optionsSetGuestBuffer(Options Options,
                                             std::string_view Name,
                                             Span<uint8_t> Value) {
  return std::visit(
      [Name, Value](auto &&Option) -> WasiCryptoExpect<void> {
        return Option->setGuestBuffer(Name, Value);
      },
      Options);
}

WasiCryptoExpect<std::vector<uint8_t>> optionsGet(Options Options,
                                                  std::string_view Name) {
  return std::visit(
      [Name](auto &&Option) -> WasiCryptoExpect<std::vector<uint8_t>> {
        return Option->get(Name);
      },
      Options);
}

WasiCryptoExpect<uint64_t> optionsGetU64(Options Options,
                                         std::string_view Name) {
  return std::visit(
      [Name](auto &&Option) -> WasiCryptoExpect<uint64_t> {
        return Option->getU64(Name);
      },
      Options);
}
} // namespace Common
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge