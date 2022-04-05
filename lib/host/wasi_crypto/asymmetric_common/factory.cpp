// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/asymmetric_common/factory.h"
#include "host/wasi_crypto/kx/alg.h"
#include "host/wasi_crypto/kx/factory.h"
#include "host/wasi_crypto/signatures/factory.h"
#include "host/wasi_crypto/signatures/signatures.h"
#include "host/wasi_crypto/utils/error.h"
#include "host/wasi_crypto/utils/hostfunction.h"
#include "host/wasi_crypto/utils/optional.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace AsymmetricCommon {

WasiCryptoExpect<FactoryVariant> makeFactory(__wasi_algorithm_type_e_t AlgType,
                                             std::string_view AlgStr) noexcept {
  // delegate to sig and kx factory
  switch (AlgType) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES: {
    return tryFrom<Signatures::Algorithm>(AlgStr).and_then(
        [](Signatures::Algorithm Alg) noexcept
        -> WasiCryptoExpect<FactoryVariant> {
          return std::visit(
              [](auto Factory) noexcept -> WasiCryptoExpect<FactoryVariant> {
                return Factory;
              },
              Signatures::makeFactory(Alg));
        });
  }
  case __WASI_ALGORITHM_TYPE_KEY_EXCHANGE: {
    return tryFrom<Kx::Algorithm>(AlgStr).and_then(
        [](Kx::Algorithm Alg) noexcept {
          return std::visit(
              [](auto Factory) noexcept -> WasiCryptoExpect<FactoryVariant> {
                return Factory;
              },
              Kx::makeFactory(Alg));
        });
  }
  case __WASI_ALGORITHM_TYPE_SYMMETRIC:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  default:
    assumingUnreachable();
  }
}

WasiCryptoExpect<PkVariant>
importPk(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
         Span<const uint8_t> Encoded,
         __wasi_publickey_encoding_e_t Encoding) noexcept {
  return makeFactory(AlgType, AlgStr)
      .and_then(
          [=](auto FactoryVariant) noexcept -> WasiCryptoExpect<PkVariant> {
            return std::visit(
                [=](auto Factory) noexcept -> WasiCryptoExpect<PkVariant> {
                  using FactoryType = std::decay_t<decltype(Factory)>;
                  return FactoryType::PublicKey::import(Encoded, Encoding);
                },
                FactoryVariant);
          });
}

WasiCryptoExpect<SkVariant>
importSk(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
         Span<const uint8_t> Encoded,
         __wasi_secretkey_encoding_e_t Encoding) noexcept {
  return makeFactory(AlgType, AlgStr)
      .and_then(
          [=](auto FactoryVariant) noexcept -> WasiCryptoExpect<SkVariant> {
            return std::visit(
                [=](auto Factory) noexcept -> WasiCryptoExpect<SkVariant> {
                  using FactoryType = std::decay_t<decltype(Factory)>;
                  return FactoryType::SecretKey::import(Encoded, Encoding);
                },
                FactoryVariant);
          });
}

WasiCryptoExpect<KpVariant>
importKp(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
         Span<const uint8_t> Encoded,
         __wasi_keypair_encoding_e_t Encoding) noexcept {
  return makeFactory(AlgType, AlgStr)
      .and_then(
          [=](auto FactoryVariant) noexcept -> WasiCryptoExpect<KpVariant> {
            return std::visit(
                [=](auto Factory) noexcept -> WasiCryptoExpect<KpVariant> {
                  using FactoryType = std::decay_t<decltype(Factory)>;
                  return FactoryType::KeyPair::import(Encoded, Encoding);
                },
                FactoryVariant);
          });
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
generateKp(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
           OptionalRef<const Common::Options> OptOptions) noexcept {
  return makeFactory(AlgType, AlgStr)
      .and_then([=](auto FactoryVariant) noexcept
                -> WasiCryptoExpect<KpVariant> {
        return std::visit(
            [=](auto Factory) noexcept -> WasiCryptoExpect<KpVariant> {
              using FactoryType = std::decay_t<decltype(Factory)>;
              using KeyPairType = typename FactoryType::KeyPair;
              using RequiredOptionsType = typename GernerateKpTrait<
                  decltype(&FactoryType::KeyPair::generate)>::Options;

              return transposeOptionalRef(
                         OptOptions,
                         [](auto &&Options)
                             -> WasiCryptoExpect<
                                 OptionalRef<RequiredOptionsType>> {
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
            FactoryVariant);
      });
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

} // namespace AsymmetricCommon
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge