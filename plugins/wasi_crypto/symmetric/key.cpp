// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "symmetric/key.h"
#include "utils/error.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

WasiCryptoExpect<KeyVariant> importKey(Algorithm Alg,
                                       Span<const uint8_t> Data) noexcept {
  return std::visit(
      [Data](auto Factory) noexcept {
        return decltype(Factory)::Key::import(Data).map(
            [](auto &&Key) noexcept {
              return KeyVariant{std::forward<decltype(Key)>(Key)};
            });
      },
      Alg);
}

WasiCryptoExpect<KeyVariant>
generateKey(Algorithm Alg, OptionalRef<const Options> OptOptions) noexcept {
  return std::visit(
      [OptOptions](auto Factory) noexcept {
        return decltype(Factory)::Key::generate(OptOptions)
            .map([](auto &&Key) noexcept {
              return KeyVariant{std::forward<decltype(Key)>(Key)};
            });
      },
      Alg);
}

SecretVec keyExportData(const KeyVariant &KeyVariant) noexcept {
  return std::visit([](const auto &Key) noexcept { return Key.exportData(); },
                    KeyVariant);
}

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
