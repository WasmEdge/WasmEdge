// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/asymmetric_common/keypair.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace AsymmetricCommon {

WasiCryptoExpect<KpVariant>
importKp(AsymmetricCommon::Algorithm Alg, Span<const uint8_t> Encoded,
         __wasi_keypair_encoding_e_t Encoding) noexcept {
  return std::visit(
      [=](auto Factory) noexcept -> WasiCryptoExpect<KpVariant> {
        using FactoryType = std::decay_t<decltype(Factory)>;
        return FactoryType::KeyPair::import(Encoded, Encoding);
      },
      Alg);
}
namespace {
template <typename P> struct GernerateKpTrait;
template <typename OptionsType, typename KeyPairType>
struct GernerateKpTrait<WasiCryptoExpect<KeyPairType> (*)(
    OptionalRef<const OptionsType>) noexcept> {
  using Options = OptionsType;
};
template <typename T>
using OptionsType =
    typename GernerateKpTrait<decltype(&T::State::generate)>::Options;
} // namespace

WasiCryptoExpect<KpVariant>
generateKp(AsymmetricCommon::Algorithm Alg,
           OptionalRef<const Common::Options> OptOptions) noexcept {
  return std::visit(
      [=](auto Factory) noexcept -> WasiCryptoExpect<KpVariant> {
        using FactoryType = std::decay_t<decltype(Factory)>;
        using KeyPairType = typename FactoryType::KeyPair;
        using RequiredOptionsType = typename GernerateKpTrait<
            decltype(&FactoryType::KeyPair::generate)>::Options;

        return transposeOptionalRef(
                   OptOptions,
                   [](auto &&Options)
                       -> WasiCryptoExpect<OptionalRef<RequiredOptionsType>> {
                     using OptionsType = std::decay_t<decltype(Options)>;
                     if constexpr (std::is_same_v<OptionsType,
                                                  RequiredOptionsType>) {
                       return Options;
                     } else {
                       return WasiCryptoUnexpect(
                           __WASI_CRYPTO_ERRNO_INVALID_HANDLE);
                     }
                   })
            .and_then([](auto OptRequiredOptions) {
              return KeyPairType::generate(OptRequiredOptions);
            });
      },
      Alg);
}

namespace {
template <typename P> struct toKeyPairTrait;
template <typename PublicKeyType, typename SecretKeyType, typename KeyPairType>
struct toKeyPairTrait<WasiCryptoExpect<KeyPairType> (SecretKeyType::*)(
    const PublicKeyType &) const noexcept> {
  using PublicKey = PublicKeyType;
};
template <typename T>
using PublicKeyType =
    typename toKeyPairTrait<decltype(&T::toKeyPair)>::PublicKey;
} // namespace

WasiCryptoExpect<KpVariant> kpFromPkAndSk(const PkVariant &PkVariant,
                                          const SkVariant &SkVariant) noexcept {
  return std::visit(
      [](const auto &Pk,
         const auto &Sk) noexcept -> WasiCryptoExpect<KpVariant> {
        using RequiredPkType = PublicKeyType<std::decay_t<decltype(Sk)>>;
        if constexpr (std::is_same_v<RequiredPkType,
                                     std::decay_t<decltype(Pk)>>) {
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
        return Kp.publicKey().map(
            [](auto &&Pk) noexcept { return PkVariant{std::move(Pk)}; });
      },
      KpVariant);
}

WasiCryptoExpect<SkVariant> kpSecretKey(const KpVariant &KpVariant) noexcept {
  return std::visit(
      [](const auto &Kp) noexcept {
        return Kp.secretKey().map(
            [](auto &&Sk) noexcept { return SkVariant{std::move(Sk)}; });
      },
      KpVariant);
}

} // namespace AsymmetricCommon
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge