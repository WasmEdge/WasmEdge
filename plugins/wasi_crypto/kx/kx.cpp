// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "kx/kx.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

namespace {
template <typename T> struct DhTrait;

template <typename SkType, typename PkType>
struct DhTrait<WasiCryptoExpect<SecretVec> (SkType::*)(const PkType &)
                   const noexcept> {
  using Pk = PkType;
};
template <typename T> using PkType = typename DhTrait<decltype(&T::dh)>::Pk;
} // namespace

WasiCryptoExpect<SecretVec> dh(const PkVariant &PkVariant,
                               const SkVariant &SkVariant) noexcept {
  return std::visit(
      [](const auto &Pk,
         const auto &Sk) noexcept -> WasiCryptoExpect<SecretVec> {
        using InPkType = std::decay_t<decltype(Pk)>;
        using ExpectPkType = PkType<std::decay_t<decltype(Sk)>>;
        if constexpr (std::is_same_v<InPkType, ExpectPkType>) {
          return Sk.dh(Pk);
        } else {
          return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
        }
      },
      PkVariant, SkVariant);
}

WasiCryptoExpect<EncapsulatedSecret>
encapsulate(PkVariant &PkVariant) noexcept {
  return std::visit(
      [](auto &&) {
        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
      },
      PkVariant);
}

WasiCryptoExpect<std::vector<uint8_t>>
decapsulate(SkVariant &, Span<const uint8_t>) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
