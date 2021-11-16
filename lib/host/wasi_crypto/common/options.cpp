// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/common/options.h"
#include "host/wasi_crypto/key_exchange/options.h"
#include "host/wasi_crypto/signature/options.h"
#include "host/wasi_crypto/symmetric/options.h"

#include <variant>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace {
template <class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;
} // namespace

WasiCryptoExpect<Options> Options::make(__wasi_algorithm_type_e_t Algorithm) {
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

WasiCryptoExpect<SignatureOptions> Options::asSignatures() {
  auto *Res = std::get_if<SignatureOptions>(&Inner);
  if (Res == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
  }

  return *Res;
}

WasiCryptoExpect<SymmetricOptions> Options::asSymmetric() {
  auto *Res = std::get_if<SymmetricOptions>(&Inner);
  if (Res == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
  }

  return *Res;
}

WasiCryptoExpect<KxOptions> Options::asKx() {
  auto *Res = std::get_if<KxOptions>(&Inner);
  if (Res == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
  }

  return *Res;
}

WasiCryptoExpect<void> Options::set(std::string_view Name,
                                    Span<const uint8_t> Value) {
  return std::visit(
      Overloaded{
          [Name, Value](SymmetricOptions Options) -> WasiCryptoExpect<void> {
            Options.set(Name, Value);
            return {};
          },
          [](auto) -> WasiCryptoExpect<void> {
            return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
          }},
      Inner);
}

WasiCryptoExpect<void> Options::setU64(std::string_view Name, uint64_t Value) {
  return std::visit(
      Overloaded{
          [Name, Value](SymmetricOptions Options) -> WasiCryptoExpect<void> {
            Options.setU64(Name, Value);
            return {};
          },
          [](auto) -> WasiCryptoExpect<void> {
            return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
          }},
      Inner);
}

WasiCryptoExpect<void> Options::setGuestBuffer(std::string_view Name,
                                               Span<uint8_t> Buffer) {
  return std::visit(
      Overloaded{
          [Name, Buffer](SymmetricOptions Options) -> WasiCryptoExpect<void> {
            Options.set(Name, Buffer);
            return {};
          },
          [](auto) -> WasiCryptoExpect<void> {
            return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
          }},
      Inner);
}

WasiCryptoExpect<std::vector<uint8_t>> Options::get(std::string_view Name) {
  return std::visit(
      Overloaded{[Name](SymmetricOptions Options)
                     -> WasiCryptoExpect<std::vector<uint8_t>> {
                   return Options.get(Name);
                 },
                 [](auto) -> WasiCryptoExpect<std::vector<uint8_t>> {
                   return WasiCryptoUnexpect(
                       __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
                 }},
      Inner);
}

WasiCryptoExpect<uint64_t> Options::getU64(std::string_view Name) {
  return std::visit(
      Overloaded{
          [Name](SymmetricOptions Options) -> WasiCryptoExpect<uint64_t> {
            return Options.getU64(Name);
          },
          [](auto) -> WasiCryptoExpect<uint64_t> {
            return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
          }},
      Inner);
}

Options::Options(
    std::variant<SymmetricOptions, SignatureOptions, KxOptions> Inner)
    : Inner(Inner) {}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge