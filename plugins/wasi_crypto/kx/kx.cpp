// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "kx/kx.h"

#include <type_traits>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

namespace {
template <typename Sk, typename Pk, typename = void>
struct IsDhCompatible : std::false_type {};

template <typename Sk, typename Pk>
struct IsDhCompatible<
    Sk, Pk, std::void_t<decltype(std::declval<const Sk &>().dh(std::declval<const Pk &>()))>>
    : std::true_type {};

// C++17 void_t SFINAE member-detection traits. Only algorithm classes that
// implement a KEM (e.g. ML-KEM) expose encapsulate/decapsulate; DH classes
// (X25519, Ecdsa) do not, so they fall through to NOT_IMPLEMENTED unchanged.
template <typename T, typename = void>
struct HasEncapsulateTrait : std::false_type {};
template <typename T>
struct HasEncapsulateTrait<
    T, std::void_t<decltype(std::declval<const T &>().encapsulate())>>
    : std::true_type {};
template <typename T>
inline constexpr bool HasEncapsulate = HasEncapsulateTrait<T>::value;

template <typename T, typename = void>
struct HasDecapsulateTrait : std::false_type {};
template <typename T>
struct HasDecapsulateTrait<
    T, std::void_t<decltype(std::declval<const T &>().decapsulate(
           std::declval<Span<const uint8_t>>()))>> : std::true_type {};
template <typename T>
inline constexpr bool HasDecapsulate = HasDecapsulateTrait<T>::value;
} // namespace

WasiCryptoExpect<SecretVec> dh(const PkVariant &PkVariant,
                               const SkVariant &SkVariant) noexcept {
  return std::visit(
      [](const auto &Pk, const auto &Sk) -> WasiCryptoExpect<SecretVec> {
        using PkType = std::decay_t<decltype(Pk)>;
        using SkType = std::decay_t<decltype(Sk)>;

        if constexpr (IsDhCompatible<SkType, PkType>::value) {
          return Sk.dh(Pk);
        } else {
          return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INCOMPATIBLE_KEYS);
        }
      },
      PkVariant, SkVariant);
}

WasiCryptoExpect<EncapsulatedSecret>
encapsulate(PkVariant &PkVariant) noexcept {
  return std::visit(
      [](auto &Pk) noexcept -> WasiCryptoExpect<EncapsulatedSecret> {
        using InPkType = std::decay_t<decltype(Pk)>;
        if constexpr (HasEncapsulate<InPkType>) {
          return Pk.encapsulate();
        } else {
          return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
        }
      },
      PkVariant);
}

WasiCryptoExpect<SecretVec>
decapsulate(SkVariant &SkVariant,
            Span<const uint8_t> EncapsulatedSecret) noexcept {
  return std::visit(
      [EncapsulatedSecret](auto &Sk) noexcept -> WasiCryptoExpect<SecretVec> {
        using InSkType = std::decay_t<decltype(Sk)>;
        if constexpr (HasDecapsulate<InSkType>) {
          return Sk.decapsulate(EncapsulatedSecret);
        } else {
          return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
        }
      },
      SkVariant);
}

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
