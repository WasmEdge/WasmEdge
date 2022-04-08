// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/utils/error.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

WasiCryptoExpect<KeyVariant> importKey(Algorithm Alg,
                                       Span<const uint8_t> Data) noexcept {
  return std::visit(
      [Data](auto Factory) noexcept {
        using KeyType = typename std::decay_t<decltype(Factory)>::Key;
        return KeyType::import(Data).map(
            [](auto &&Key) noexcept { return KeyVariant{std::move(Key)}; });
      },
      Alg);
}

WasiCryptoExpect<KeyVariant>
generateKey(Algorithm Alg, OptionalRef<const Options> OptOptions) noexcept {
  return std::visit(
      [OptOptions](auto Factory) mutable noexcept {
        using KeyType = typename std::decay_t<decltype(Factory)>::Key;
        return KeyType::generate(OptOptions).map([](auto &&Key) noexcept {
          return KeyVariant{std::move(Key)};
        });
      },
      Alg);
}

std::vector<uint8_t> keyExportData(const KeyVariant &KeyVariant) noexcept {
  return std::visit([](const auto &Key) noexcept { return Key.exportData(); },
                    KeyVariant);
}

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge