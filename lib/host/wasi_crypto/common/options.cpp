// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/common/options.h"
#include "host/wasi_crypto/key_exchange/options.h"
#include "host/wasi_crypto/signature/options.h"
#include "host/wasi_crypto/symmetric/options.h"

#include <variant>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<void> Options::set(std::string_view Name,
                                    Span<const uint8_t> Value) {
  return std::visit(Overloaded{[&Name, &Value](SymmetricOptions &Options) {
                                 return Options.inner()->locked(
                                     [&Name, &Value](auto &Inner) {
                                       return Inner.set(Name, Value);
                                     });
                               },
                               [](auto &) -> WasiCryptoExpect<void> {
                                 return WasiCryptoUnexpect(
                                     __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
                               }},
                    Inner);
}

WasiCryptoExpect<void> Options::setU64(std::string_view Name, uint64_t Value) {
  return std::visit(Overloaded{[&Name, Value](SymmetricOptions Options) {
                                 return Options.inner()->locked(
                                     [&Name, Value](auto &Inner) {
                                       return Inner.setU64(Name, Value);
                                     });
                               },
                               [](auto) -> WasiCryptoExpect<void> {
                                 return WasiCryptoUnexpect(
                                     __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
                               }},
                    Inner);
}

WasiCryptoExpect<void> Options::setGuestBuffer(std::string_view Name,
                                               Span<uint8_t> Buffer) {
  return std::visit(
      Overloaded{
          [Name, Buffer](SymmetricOptions Options) {
            return Options.inner()->locked([&Name, &Buffer](auto &Inner) {
              return Inner.setGuestBuffer(Name, Buffer);
            });
          },
          [](auto) -> WasiCryptoExpect<void> {
            return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
          }},
      Inner);
}

WasiCryptoExpect<std::vector<uint8_t>> Options::get(std::string_view Name) {
  return std::visit(
      Overloaded{[Name](SymmetricOptions &Options) {
                   return Options.inner()->locked(
                       [&Name](auto &Inner) { return Inner.get(Name); });
                 },
                 [](auto &) -> WasiCryptoExpect<std::vector<uint8_t>> {
                   return WasiCryptoUnexpect(
                       __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
                 }},
      Inner);
}

WasiCryptoExpect<uint64_t> Options::getU64(std::string_view Name) {
  return std::visit(
      Overloaded{
          [Name](SymmetricOptions &Options) -> WasiCryptoExpect<uint64_t> {
            return Options.inner()->locked(
                [&Name](auto &Inner) { return Inner.getU64(Name); });
          },
          [](auto &) -> WasiCryptoExpect<uint64_t> {
            return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
          }},
      Inner);
}

WasiCryptoExpect<Options>
Options::generate(__wasi_algorithm_type_e_t Algorithm) {
  switch (Algorithm) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES:
    return Options{SignatureOptions{}};
  case __WASI_ALGORITHM_TYPE_SYMMETRIC:
    return Options{SymmetricOptions{}};
  case __WASI_ALGORITHM_TYPE_KEY_EXCHANGE:
    return Options{KxOptions{}};
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  }
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge