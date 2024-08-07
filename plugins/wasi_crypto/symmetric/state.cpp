// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "symmetric/state.h"
#include "utils/error.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {
namespace {
WasiCryptoExpect<size_t> checkedAdd(size_t A, size_t B) {
  size_t Res;
  ensureOrReturn(!__builtin_add_overflow(A, B, &Res),
                 __WASI_CRYPTO_ERRNO_OVERFLOW);
  return Res;
}

/// Correspond signatures:
template <typename P> struct StateOpenTrait;

/// WasiCryptoExpect<StateType> open(const KeyType &, OptionalRef<const
/// OptionsType>);
template <typename StateType, typename KeyType, typename OptionsType>
struct StateOpenTrait<WasiCryptoExpect<StateType> (*)(
    const KeyType &, OptionalRef<const OptionsType>) noexcept> {
  static inline constexpr bool NeedKey = true;
  using Key = KeyType;
};

/// WasiCryptoExpect<StateType> open(OptionalRef<const OptionsType>);
template <typename StateType, typename OptionsType>
struct StateOpenTrait<WasiCryptoExpect<StateType> (*)(
    OptionalRef<const OptionsType>) noexcept> {
  static inline constexpr bool NeedKey = false;
};

template <typename T>
using GetStateOpenTrait = StateOpenTrait<decltype(&T::State::open)>;
} // namespace

WasiCryptoExpect<StateVariant>
openState(Algorithm Alg, OptionalRef<const KeyVariant> OptKeyVariant,
          OptionalRef<const Options> OptOptions) noexcept {
  return std::visit(
      [=](auto Factory) noexcept -> WasiCryptoExpect<StateVariant> {
        using StateOpen = GetStateOpenTrait<decltype(Factory)>;
        if constexpr (StateOpen::NeedKey) {
          using RequiredKeyType = typename StateOpen::Key;
          // Need key. Not have key, fail.
          if (unlikely(!OptKeyVariant)) {
            return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_KEY_REQUIRED);
          }
          return std::visit(
              [OptOptions](const auto &Key) -> WasiCryptoExpect<StateVariant> {
                using InKeyType = std::decay_t<decltype(Key)>;
                if constexpr (!std::is_same_v<InKeyType, RequiredKeyType>) {
                  // Key type not same.
                  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
                } else {
                  // Key type fitted.
                  return decltype(Factory)::State::open(Key, OptOptions)
                      .map([](auto &&State) noexcept {
                        return StateVariant{
                            std::forward<decltype(State)>(State)};
                      });
                }
              },
              *OptKeyVariant);

        } else {
          // Not need key. Have key, fail.
          if (unlikely(!!OptKeyVariant)) {
            return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_KEY_NOT_SUPPORTED);
          }
          return decltype(Factory)::State::open(OptOptions)
              .map([](auto &&State) noexcept {
                return StateVariant{std::forward<decltype(State)>(State)};
              });
        }
      },
      Alg);
}

WasiCryptoExpect<size_t> stateOptionsGet(const StateVariant &StateVariant,
                                         std::string_view Name,
                                         Span<uint8_t> Value) noexcept {
  return std::visit(
      [=](const auto &State) noexcept { return State.optionsGet(Name, Value); },
      StateVariant);
}

WasiCryptoExpect<uint64_t> stateOptionsGetU64(const StateVariant &StateVariant,
                                              std::string_view Name) noexcept {
  return std::visit(
      [Name](const auto &State) noexcept { return State.optionsGetU64(Name); },
      StateVariant);
}

WasiCryptoExpect<void> stateAbsorb(StateVariant &StateVariant,
                                   Span<const uint8_t> Data) noexcept {
  return std::visit([Data](auto &State) noexcept { return State.absorb(Data); },
                    StateVariant);
}

WasiCryptoExpect<void> stateSqueeze(StateVariant &StateVariant,
                                    Span<uint8_t> Out) noexcept {
  return std::visit([Out](auto &State) noexcept { return State.squeeze(Out); },
                    StateVariant);
}

WasiCryptoExpect<Tag> stateSqueezeTag(StateVariant &StateVariant) noexcept {
  return std::visit([](auto &State) noexcept { return State.squeezeTag(); },
                    StateVariant);
}

namespace {
template <typename> struct GetSqueezeKeyTypeTrait;
template <typename KeyType, typename StateType>
struct GetSqueezeKeyTypeTrait<WasiCryptoExpect<KeyType> (
    StateType::*)() noexcept> {
  using Key = KeyType;
};
template <typename T>
using GetSqueezeKeyType =
    typename GetSqueezeKeyTypeTrait<decltype(&T::squeezeKey)>::Key;

} // namespace

WasiCryptoExpect<KeyVariant> stateSqueezeKey(StateVariant &StateVariant,
                                             Algorithm KeyAlg) noexcept {
  return std::visit(
      [](auto &State, auto Alg) noexcept -> WasiCryptoExpect<KeyVariant> {
        if constexpr (std::is_same_v<
                          GetSqueezeKeyType<std::decay_t<decltype(State)>>,
                          typename decltype(Alg)::Key>) {
          return State.squeezeKey().map([](auto &&Key) {
            return KeyVariant{std::forward<decltype(Key)>(Key)};
          });
        } else {
          return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
        }
      },
      StateVariant, KeyAlg);
}

WasiCryptoExpect<size_t>
stateMaxTagLen(const StateVariant &StateVariant) noexcept {
  return std::visit(
      [](const auto &State) noexcept { return State.maxTagLen(); },
      StateVariant);
}

WasiCryptoExpect<size_t> stateEncrypt(StateVariant &StateVariant,
                                      Span<uint8_t> Out,
                                      Span<const uint8_t> Data) noexcept {
  return std::visit(
      [=](auto &State) noexcept -> WasiCryptoExpect<size_t> {
        return State.maxTagLen()
            .and_then([DataSize = Data.size()](size_t TagLen) noexcept {
              return checkedAdd(DataSize, TagLen);
            })
            .and_then([Out, Data, &State](size_t ActualDataLen) noexcept
                      -> WasiCryptoExpect<size_t> {
              ensureOrReturn(Out.size() == ActualDataLen,
                             __WASI_CRYPTO_ERRNO_INVALID_LENGTH);
              return State.encrypt(Out, Data);
            });
      },
      StateVariant);
}

WasiCryptoExpect<Tag> stateEncryptDetached(StateVariant &StateVariant,
                                           Span<uint8_t> Out,
                                           Span<const uint8_t> Data) noexcept {
  ensureOrReturn(Data.size() == Out.size(), __WASI_CRYPTO_ERRNO_INVALID_LENGTH);
  return std::visit(
      [=](auto &State) noexcept { return State.encryptDetached(Out, Data); },
      StateVariant);
}

WasiCryptoExpect<size_t> stateDecrypt(StateVariant &StateVariant,
                                      Span<uint8_t> Out,
                                      Span<const uint8_t> Data) noexcept {
  return std::visit(
      [=](auto &State) noexcept -> WasiCryptoExpect<size_t> {
        return State.maxTagLen()
            .and_then([OutSize = Out.size()](size_t TagLen) noexcept {
              return checkedAdd(OutSize, TagLen);
            })
            .and_then([Out, Data, &State](size_t ActualOutLen) noexcept
                      -> WasiCryptoExpect<size_t> {
              ensureOrReturn(Data.size() == ActualOutLen,
                             __WASI_CRYPTO_ERRNO_INVALID_LENGTH);
              return State.decrypt(Out, Data);
            });
      },
      StateVariant);
}

WasiCryptoExpect<size_t>
stateDecryptDetached(StateVariant &StateVariant, Span<uint8_t> Out,
                     Span<const uint8_t> Data,
                     Span<const uint8_t> RawTag) noexcept {
  ensureOrReturn(Data.size() == Out.size(), __WASI_CRYPTO_ERRNO_INVALID_LENGTH);
  return std::visit(
      [=](auto &State) noexcept {
        return State.decryptDetached(Out, Data, RawTag);
      },
      StateVariant);
}

WasiCryptoExpect<void> stateRatchet(StateVariant &StateVariant) noexcept {
  return std::visit([](auto &State) noexcept { return State.ratchet(); },
                    StateVariant);
}

WasiCryptoExpect<StateVariant>
stateClone(const StateVariant &ClonedStateVariant) noexcept {
  return std::visit(
      [](const auto &ClonedState) noexcept -> WasiCryptoExpect<StateVariant> {
        return ClonedState.clone().map([](auto &&NewState) noexcept {
          return StateVariant{std::forward<decltype(NewState)>(NewState)};
        });
      },
      ClonedStateVariant);
}

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
