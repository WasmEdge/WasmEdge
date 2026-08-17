// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "kx/kx.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

WasiCryptoExpect<SecretVec> dh(const PkVariant &PkVariant,
                               const SkVariant &SkVariant) noexcept {
  return std::visit(
      [](const auto &Pk, const auto &Sk) -> WasiCryptoExpect<SecretVec> {
        using PkType = std::decay_t<decltype(Pk)>;
        using SkType = std::decay_t<decltype(Sk)>;
        if constexpr (std::is_same_v<PkType, X25519::PublicKey> &&
                      std::is_same_v<SkType, X25519::SecretKey>) {
          return Sk.dh(Pk);
        } else if constexpr (std::is_same_v<PkType, EcdsaP256::PublicKey> &&
                             std::is_same_v<SkType, EcdsaP256::SecretKey>) {
          return Sk.dh(Pk);
        } else if constexpr (std::is_same_v<PkType, EcdsaP384::PublicKey> &&
                             std::is_same_v<SkType, EcdsaP384::SecretKey>) {
          return Sk.dh(Pk);
        } else {
          return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INCOMPATIBLE_KEYS);
        }
      },
      PkVariant, SkVariant);
}

WasiCryptoExpect<EncapsulatedSecret> encapsulate(PkVariant &) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
}

WasiCryptoExpect<std::vector<uint8_t>>
decapsulate(SkVariant &, Span<const uint8_t>) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
}

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
