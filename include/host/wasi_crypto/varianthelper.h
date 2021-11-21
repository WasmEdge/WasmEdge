// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/error.h"
#include "wasi_crypto/api.hpp"
#include <variant>
namespace WasmEdge {
namespace Host {
namespace WASICrypto {

template <class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

template <typename... Ts> class VariantTemplate {
public:
  VariantTemplate(std::variant<Ts...> Inner) : Inner(Inner) {}

  template <typename T,
            std::enable_if_t<(std::is_same_v<T, Ts> || ...), bool> = true>
  constexpr WasiCryptoExpect<T> as() noexcept {
    auto *Res = std::get_if<T>(&Inner);
    if (Res == nullptr) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
    }
    return *Res;
  }

protected:
  std::variant<Ts...> Inner;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge