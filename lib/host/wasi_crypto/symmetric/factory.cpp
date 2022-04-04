// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/symmetric/factory.h"
#include "host/wasi_crypto/symmetric/kdf.h"
#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/state.h"
#include "host/wasi_crypto/utils/error.h"
#include "wasi_crypto/api.hpp"

#include <optional>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

FactoryVariant makeFactory(Algorithm Alg) noexcept {
  switch (Alg) {
  case Algorithm::Sha256:
    return FactoryVariant{std::in_place_type<Sha256>};
  case Algorithm::Sha512:
    return FactoryVariant{std::in_place_type<Sha512>};
  case Algorithm::Sha512_256:
    return FactoryVariant{std::in_place_type<Sha512_256>};
  case Algorithm::HmacSha256:
    return FactoryVariant{std::in_place_type<HmacSha256>};
  case Algorithm::HmacSha512:
    return FactoryVariant{std::in_place_type<HmacSha512>};
  case Algorithm::HkdfSha256Expand:
    return FactoryVariant{std::in_place_type<HkdfSha256Expand>};
  case Algorithm::HkdfSha256Extract:
    return FactoryVariant{std::in_place_type<HkdfSha256Extract>};
  case Algorithm::HkdfSha512Expand:
    return FactoryVariant{std::in_place_type<HkdfSha512Expand>};
  case Algorithm::HkdfSha512Extract:
    return FactoryVariant{std::in_place_type<HkdfSha512Extract>};
  case Algorithm::Aes128Gcm:
    return FactoryVariant{std::in_place_type<Aes128Gcm>};
  case Algorithm::Aes256Gcm:
    return FactoryVariant{std::in_place_type<Aes256Gcm>};
  case Algorithm::ChaCha20Poly1305:
    return FactoryVariant{std::in_place_type<ChaCha20Poly1305>};
  default:
    assumingUnreachable();
  }
}

WasiCryptoExpect<KeyVariant> importKey(Algorithm Alg,
                                       Span<const uint8_t> Data) noexcept {
  return std::visit(
      [Data](auto Factory) noexcept {
        using KeyType = typename std::decay_t<decltype(Factory)>::Key;
        return KeyType::import(Data).map(
            [](auto &&Key) noexcept { return KeyVariant{Key}; });
      },
      makeFactory(Alg));
}

WasiCryptoExpect<KeyVariant>
generateKey(Algorithm Alg, OptionalRef<Options> OptOptions) noexcept {
  return std::visit(
      [=](auto Factory) mutable noexcept {
        using KeyType = typename std::decay_t<decltype(Factory)>::Key;
        return KeyType::generate(OptOptions).map([](auto &&Key) noexcept {
          return KeyVariant{Key};
        });
      },
      makeFactory(Alg));
}

namespace {
template <typename P> struct StateOpenTraits;
template <typename StateType, typename KeyType, typename OptionsType>
struct StateOpenTraits<WasiCryptoExpect<StateType> (*)(
    KeyType &, OptionalRef<OptionsType>) noexcept> {
  static inline constexpr bool NeedKey = true;
};

template <typename StateType, typename OptionsType>
struct StateOpenTraits<WasiCryptoExpect<StateType> (*)(
    OptionalRef<OptionsType>) noexcept> {
  static inline constexpr bool NeedKey = false;
};

template <typename T>
inline constexpr bool RequireKey =
    StateOpenTraits<decltype(&T::State::open)>::NeedKey;
} // namespace

WasiCryptoExpect<StateVariant>
openState(Algorithm Alg, OptionalRef<KeyVariant> OptKeyVariant,
          OptionalRef<Options> OptOptions) noexcept {
  return std::visit(
      [=](auto Factory) noexcept -> WasiCryptoExpect<StateVariant> {
        using FactoryType = std::decay_t<decltype(Factory)>;
        using KeyType = typename FactoryType::Key;
        using StateType = typename FactoryType::State;
        /// need key
        if constexpr (RequireKey<FactoryType>) {
          /// have key
          if (OptKeyVariant) {
            return std::visit(
                [OptOptions](auto &&Key) -> WasiCryptoExpect<StateVariant> {
                  /// key type not same
                  if constexpr (!std::is_same_v<std::decay_t<decltype(Key)>,
                                                KeyType>) {
                    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
                  } else {
                    /// key type same
                    return StateType::open(Key, OptOptions)
                        .map([](auto &&State) noexcept {
                          return StateVariant{State};
                        });
                  }
                },
                *OptKeyVariant);
          }
          ///  not have key
          return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_KEY_REQUIRED);

        } else {
          /// not need key

          /// have key
          if (OptKeyVariant) {
            return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_KEY_NOT_SUPPORTED);
          }

          /// not have key
          return StateType::open(OptOptions).map([](auto &&State) noexcept {
            return StateVariant{State};
          });
        }
      },
      makeFactory(Alg));
}

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge