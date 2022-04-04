// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/symmetric/state.h"
#include "host/wasi_crypto/utils/error.h"
#include "wasi_crypto/api.hpp"

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
} // namespace

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

WasiCryptoExpect<KeyVariant> stateSqueezeKey(StateVariant &StateVariant,
                                             Algorithm KeyAlg) noexcept {
  return std::visit(
      [KeyAlg](auto &State) noexcept {
        return State.squeezeKey(KeyAlg).map(
            [](auto &&Key) { return KeyVariant{std::move(Key)}; });
      },
      StateVariant);
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

WasiCryptoExpect<size_t> stateDecryptDetached(StateVariant &StateVariant,
                                              Span<uint8_t> Out,
                                              Span<const uint8_t> Data,
                                              Span<uint8_t> RawTag) noexcept {
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

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge