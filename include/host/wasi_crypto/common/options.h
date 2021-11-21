// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/key_exchange/options.h"
#include "host/wasi_crypto/signature/options.h"
#include "host/wasi_crypto/symmetric/options.h"
#include "host/wasi_crypto/varianthelper.h"
#include "wasi_crypto/api.hpp"

#include <memory>
#include <string_view>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class Options
    : public VariantTemplate<SymmetricOptions, SignatureOptions, KxOptions> {
public:
  using VariantTemplate<SymmetricOptions, SignatureOptions, KxOptions>::VariantTemplate;

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

  WasiCryptoExpect<std::vector<uint8_t>> get(std::string_view Name);

  WasiCryptoExpect<uint64_t> getU64(std::string_view Name);

public:
  static WasiCryptoExpect<Options> make(__wasi_algorithm_type_e_t Algorithm);

};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge