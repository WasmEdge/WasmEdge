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
  using VariantTemplate<SymmetricOptions, SignatureOptions,
                        KxOptions>::VariantTemplate;

  WasiCryptoExpect<void> set(std::string_view Name, Span<const uint8_t> Value);

  WasiCryptoExpect<void> setU64(std::string_view Name, uint64_t Value);

  WasiCryptoExpect<void> setGuestBuffer(std::string_view Name,
                                        Span<uint8_t> Buffer);

  WasiCryptoExpect<std::vector<uint8_t>> get(std::string_view Name);

  WasiCryptoExpect<uint64_t> getU64(std::string_view Name);

  static WasiCryptoExpect<Options>
  generate(__wasi_algorithm_type_e_t Algorithm);
};

template <typename T>
WasiCryptoExpect<std::optional<T>>
optOptionsAs(std::optional<Options> OptOptions) {
  if (OptOptions) {
    auto Res = OptOptions->template as<T>();
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }

    return std::make_optional(*Res);
  }
  return std::nullopt;
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge