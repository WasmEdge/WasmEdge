// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/key_exchange/options.h"
#include "host/wasi_crypto/signature/options.h"
#include "host/wasi_crypto/symmetric/options.h"
#include "wasi_crypto/api.hpp"

#include <memory>
#include <string_view>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace {
template <class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;
} // namespace

template <typename... Ts> class OptionTemplate {
public:
  OptionTemplate(std::variant<Ts...> Inner) : Inner(Inner) {}

  template <typename T,
            std::enable_if_t<(std::is_same_v<T, Ts> || ...), bool> = true>
  constexpr WasiCryptoExpect<T> as() noexcept {
    auto *Res = std::get_if<T>(&Inner);
    if (Res == nullptr) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
    }
    return *Res;
  }

  constexpr WasiCryptoExpect<void> set(std::string_view Name,
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

  constexpr WasiCryptoExpect<void> setU64(std::string_view Name,
                                          uint64_t Value) {
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

  constexpr WasiCryptoExpect<void> setGuestBuffer(std::string_view Name,
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

  WasiCryptoExpect<std::vector<uint8_t>> get(std::string_view Name) {
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

  WasiCryptoExpect<uint64_t> getU64(std::string_view Name) {
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

private:
  std::variant<Ts...> Inner;
};

class Options
    : public OptionTemplate<SymmetricOptions, SignatureOptions, KxOptions> {
  using OptionTemplate<SymmetricOptions, SignatureOptions, KxOptions>::OptionTemplate;

public:
  static WasiCryptoExpect<Options> make(__wasi_algorithm_type_e_t Algorithm);
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge