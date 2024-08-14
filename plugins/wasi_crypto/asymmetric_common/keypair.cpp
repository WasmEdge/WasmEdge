// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "asymmetric_common/keypair.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace AsymmetricCommon {

WasiCryptoExpect<KpVariant>
importKp(AsymmetricCommon::Algorithm Alg, Span<const uint8_t> Encoded,
         __wasi_keypair_encoding_e_t Encoding) noexcept {
  return std::visit(
      [=](auto Factory) noexcept -> WasiCryptoExpect<KpVariant> {
        return decltype(Factory)::KeyPair::import(Encoded, Encoding);
      },
      Alg);
}

namespace {
/// Correspond signatures:
///   WasiCryptoExpect<KeyPairType> generate(OptionalRef<const OptionsType>);
/// is used to get the `OptionsType`.
template <typename T> struct KpGenerateTrait;
template <typename OptionsType, typename KeyPairType>
struct KpGenerateTrait<WasiCryptoExpect<KeyPairType> (*)(
    OptionalRef<const OptionsType>) noexcept> {
  using Options = OptionsType;
};
template <typename T>
using OptionsType =
    typename KpGenerateTrait<decltype(&T::KeyPair::generate)>::Options;
} // namespace

WasiCryptoExpect<KpVariant>
generateKp(AsymmetricCommon::Algorithm Alg,
           OptionalRef<const Common::Options> OptOptions) noexcept {
  return std::visit(
      [=](auto Factory) noexcept -> WasiCryptoExpect<KpVariant> {
        using RequiredOptionsType = OptionsType<decltype(Factory)>;
        return transposeOptionalRef(
                   OptOptions,
                   [](auto &&Options) noexcept
                   -> WasiCryptoExpect<OptionalRef<RequiredOptionsType>> {
                     using InOptionsType = std::decay_t<decltype(Options)>;
                     if constexpr (std::is_same_v<InOptionsType,
                                                  RequiredOptionsType>) {
                       return Options;
                     } else {
                       return WasiCryptoUnexpect(
                           __WASI_CRYPTO_ERRNO_INVALID_HANDLE);
                     }
                   })
            .and_then([](auto OptRequiredOptions) noexcept {
              return decltype(Factory)::KeyPair::generate(OptRequiredOptions);
            });
      },
      Alg);
}

namespace {
/// Correspond signatures:
///   WasiCryptoExpect<KeyPairType> Sk::toKeyPair(const PublicKeyType&);
/// is used to get the `PublicKeyType`.
template <typename T> struct KpFromPkAndSkTrait;
template <typename PublicKeyType, typename SecretKeyType, typename KeyPairType>
struct KpFromPkAndSkTrait<WasiCryptoExpect<KeyPairType> (SecretKeyType::*)(
    const PublicKeyType &) const noexcept> {
  using PublicKey = PublicKeyType;
};
template <typename T>
using PkType = typename KpFromPkAndSkTrait<decltype(&T::toKeyPair)>::PublicKey;
} // namespace

WasiCryptoExpect<KpVariant> kpFromPkAndSk(const PkVariant &PkVariant,
                                          const SkVariant &SkVariant) noexcept {
  return std::visit(
      [](const auto &Pk,
         const auto &Sk) noexcept -> WasiCryptoExpect<KpVariant> {
        using RequiredPkType = PkType<std::decay_t<decltype(Sk)>>;
        using InPkType = std::decay_t<decltype(Pk)>;
        if constexpr (std::is_same_v<RequiredPkType, InPkType>) {
          return Sk.toKeyPair(Pk);
        } else {
          return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
        }
      },
      PkVariant, SkVariant);
}

WasiCryptoExpect<SecretVec>
kpExportData(const KpVariant &KpVariant,
             __wasi_keypair_encoding_e_t Encoding) noexcept {
  return std::visit(
      [Encoding](const auto &Kp) noexcept { return Kp.exportData(Encoding); },
      KpVariant);
}

WasiCryptoExpect<PkVariant> kpPublicKey(const KpVariant &KpVariant) noexcept {
  return std::visit(
      [](const auto &Kp) noexcept {
        return Kp.publicKey().map([](auto &&Pk) noexcept {
          return PkVariant{std::forward<decltype(Pk)>(Pk)};
        });
      },
      KpVariant);
}

WasiCryptoExpect<SkVariant> kpSecretKey(const KpVariant &KpVariant) noexcept {
  return std::visit(
      [](const auto &Kp) noexcept {
        return Kp.secretKey().map([](auto &&Sk) noexcept {
          return SkVariant{std::forward<decltype(Sk)>(Sk)};
        });
      },
      KpVariant);
}

} // namespace AsymmetricCommon
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
