// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ctx.h"
#include "symmetric/key.h"
#include "symmetric/state.h"
#include "symmetric/tag.h"

#include <utility>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

WasiCryptoExpect<size_t>
Context::symmetricTagLen(__wasi_symmetric_tag_t TagHandle) noexcept {
  return SymmetricTagManager.get(TagHandle).map(&Symmetric::Tag::len);
}

WasiCryptoExpect<size_t>
Context::symmetricTagPull(__wasi_symmetric_tag_t TagHandle,
                          Span<uint8_t> Buf) noexcept {
  return SymmetricTagManager.get(TagHandle).and_then(
      [Buf](const Symmetric::Tag &Tag) noexcept { return Tag.pull(Buf); });
}

WasiCryptoExpect<void>
Context::symmetricTagVerify(__wasi_symmetric_tag_t TagHandle,
                            Span<const uint8_t> RawTag) noexcept {
  return SymmetricTagManager.get(TagHandle).and_then(
      [RawTag](const Symmetric::Tag &Tag) noexcept {
        return Tag.verify(RawTag);
      });
}

WasiCryptoExpect<void>
Context::symmetricTagClose(__wasi_symmetric_tag_t TagHandle) noexcept {
  return SymmetricTagManager.close(TagHandle);
}

WasiCryptoExpect<__wasi_array_output_t>
Context::symmetricKeyExport(__wasi_symmetric_key_t KeyHandle) noexcept {
  return SymmetricKeyManager.get(KeyHandle)
      .map(Symmetric::keyExportData)
      .and_then([this](auto &&Data) noexcept {
        return ArrayOutputManager.registerManager(
            std::forward<decltype(Data)>(Data));
      });
}

WasiCryptoExpect<void>
Context::symmetricKeyClose(__wasi_symmetric_key_t SymmetricKey) noexcept {
  return SymmetricKeyManager.close(SymmetricKey);
}

WasiCryptoExpect<size_t>
Context::symmetricStateOptionsGet(__wasi_symmetric_state_t StateHandle,
                                  std::string_view Name,
                                  Span<uint8_t> Value) noexcept {
  return SymmetricStateManager.get(StateHandle)
      .and_then([=](auto &&State) noexcept {
        return Symmetric::stateOptionsGet(std::forward<decltype(State)>(State),
                                          Name, Value);
      });
}

WasiCryptoExpect<uint64_t>
Context::symmetricStateOptionsGetU64(__wasi_symmetric_state_t StateHandle,
                                     std::string_view Name) noexcept {
  return SymmetricStateManager.get(StateHandle)
      .and_then([Name](auto &&State) noexcept {
        return Symmetric::stateOptionsGetU64(
            std::forward<decltype(State)>(State), Name);
      });
}

WasiCryptoExpect<void>
Context::symmetricStateClose(__wasi_symmetric_state_t StateHandle) noexcept {
  return SymmetricStateManager.close(StateHandle);
}

WasiCryptoExpect<void>
Context::symmetricStateAbsorb(__wasi_symmetric_state_t StateHandle,
                              Span<const uint8_t> Data) noexcept {
  return SymmetricStateManager.get(StateHandle)
      .and_then([Data](auto &&State) noexcept {
        return Symmetric::stateAbsorb(State, Data);
      });
}

WasiCryptoExpect<void>
Context::symmetricStateSqueeze(__wasi_symmetric_state_t StateHandle,
                               Span<uint8_t> Out) noexcept {
  return SymmetricStateManager.get(StateHandle)
      .and_then([Out](auto &&State) noexcept {
        return Symmetric::stateSqueeze(State, Out);
      });
}

WasiCryptoExpect<__wasi_symmetric_tag_t> Context::symmetricStateSqueezeTag(
    __wasi_symmetric_state_t StateHandle) noexcept {
  return SymmetricStateManager.get(StateHandle)
      .and_then([](auto &&State) noexcept {
        return Symmetric::stateSqueezeTag(State);
      })
      .and_then([this](auto &&Tag) {
        return SymmetricTagManager.registerManager(
            std::forward<decltype(Tag)>(Tag));
      });
}

WasiCryptoExpect<__wasi_symmetric_key_t>
Context::symmetricStateSqueezeKey(__wasi_symmetric_state_t StateHandle,
                                  Symmetric::Algorithm Alg) noexcept {
  return SymmetricStateManager.get(StateHandle)
      .and_then([Alg](auto &&State) noexcept {
        return Symmetric::stateSqueezeKey(State, Alg);
      })
      .and_then([this](auto &&Key) {
        return SymmetricKeyManager.registerManager(
            std::forward<decltype(Key)>(Key));
      });
}

WasiCryptoExpect<size_t> Context::symmetricStateMaxTagLen(
    __wasi_symmetric_state_t StateHandle) noexcept {
  return SymmetricStateManager.get(StateHandle)
      .and_then(&Symmetric::stateMaxTagLen);
}

WasiCryptoExpect<size_t>
Context::symmetricStateEncrypt(__wasi_symmetric_state_t StateHandle,
                               Span<uint8_t> Out,
                               Span<const uint8_t> Data) noexcept {
  return SymmetricStateManager.get(StateHandle)
      .and_then([=](auto &&State) noexcept {
        return Symmetric::stateEncrypt(State, Out, Data);
      });
}

WasiCryptoExpect<__wasi_symmetric_tag_t>
Context::symmetricStateEncryptDetached(__wasi_symmetric_state_t StateHandle,
                                       Span<uint8_t> Out,
                                       Span<const uint8_t> Data) noexcept {
  return SymmetricStateManager.get(StateHandle)
      .and_then([=](auto &&State) noexcept {
        return Symmetric::stateEncryptDetached(State, Out, Data);
      })
      .and_then([this](auto &&Tag) noexcept {
        return SymmetricTagManager.registerManager(
            std::forward<decltype(Tag)>(Tag));
      });
}

WasiCryptoExpect<size_t>
Context::symmetricStateDecrypt(__wasi_symmetric_state_t StateHandle,
                               Span<uint8_t> Out,
                               Span<const uint8_t> Data) noexcept {
  return SymmetricStateManager.get(StateHandle)
      .and_then([=](auto &&State) noexcept {
        return Symmetric::stateDecrypt(State, Out, Data);
      });
}

WasiCryptoExpect<size_t> Context::symmetricStateDecryptDetached(
    __wasi_symmetric_state_t StateHandle, Span<uint8_t> Out,
    Span<const uint8_t> Data, Span<const uint8_t> RawTag) noexcept {
  return SymmetricStateManager.get(StateHandle)
      .and_then([=](auto &&State) noexcept {
        return Symmetric::stateDecryptDetached(State, Out, Data, RawTag);
      });
}

WasiCryptoExpect<void>
Context::symmetricStateRatchet(__wasi_symmetric_state_t StateHandle) noexcept {
  return SymmetricStateManager.get(StateHandle)
      .and_then(
          [](auto &&State) noexcept { return Symmetric::stateRatchet(State); });
}

WasiCryptoExpect<__wasi_symmetric_key_t>
Context::symmetricKeyImport(Symmetric::Algorithm Alg,
                            Span<const uint8_t> Raw) noexcept {
  return Symmetric::importKey(Alg, Raw).and_then([this](auto &&Key) noexcept {
    return SymmetricKeyManager.registerManager(
        std::forward<decltype(Key)>(Key));
  });
}

WasiCryptoExpect<__wasi_symmetric_key_t>
Context::symmetricKeyGenerate(Symmetric::Algorithm Alg,
                              __wasi_opt_options_t OptOptionsHandle) noexcept {
  auto OptOptionsResult = mapAndTransposeOptional(
      OptOptionsHandle, [this](__wasi_options_t OptionsHandle) noexcept {
        return OptionsManager.get(OptionsHandle);
      });
  if (!OptOptionsResult) {
    return WasiCryptoUnexpect(OptOptionsResult);
  }

  // Refer to OptOptionsResult if it's a Symmetric::Options.
  auto OptSymmetricOptionsResult = transposeOptionalToRef(
      *OptOptionsResult,
      [](const auto &Options) noexcept
      -> WasiCryptoExpect<OptionalRef<const Symmetric::Options>> {
        auto *SymmetricOptions = std::get_if<Symmetric::Options>(&Options);
        if (!SymmetricOptions) {
          return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
        }
        return SymmetricOptions;
      });
  if (!OptSymmetricOptionsResult) {
    return WasiCryptoUnexpect(OptSymmetricOptionsResult);
  }

  return Symmetric::generateKey(Alg, *OptSymmetricOptionsResult)
      .and_then([this](auto &&Key) noexcept {
        return SymmetricKeyManager.registerManager(
            std::forward<decltype(Key)>(Key));
      });
}

WasiCryptoExpect<__wasi_symmetric_state_t>
Context::symmetricStateOpen(Symmetric::Algorithm Alg,
                            __wasi_opt_symmetric_key_t OptKeyHandle,
                            __wasi_opt_options_t OptOptionsHandle) noexcept {
  // Copy from KeyManager.
  auto OptKeyResult =
      mapAndTransposeOptional(OptKeyHandle,
                              [this](__wasi_symmetric_key_t KeyHandle) noexcept
                              -> WasiCryptoExpect<Symmetric::KeyVariant> {
                                return SymmetricKeyManager.get(KeyHandle);
                              });
  if (!OptKeyResult) {
    return WasiCryptoUnexpect(OptKeyResult);
  }

  // Copy from OptionsManager.
  auto OptOptionsResult = mapAndTransposeOptional(
      OptOptionsHandle, [this](__wasi_options_t OptionsHandle) noexcept {
        return OptionsManager.get(OptionsHandle);
      });
  if (!OptOptionsResult) {
    return WasiCryptoUnexpect(OptOptionsResult);
  }

  // Refer to OptOptionsResult if it's a Smmetric::Options.
  auto OptSymmetricOptionsResult = transposeOptionalToRef(
      *OptOptionsResult,
      [](const auto &Options) noexcept
      -> WasiCryptoExpect<OptionalRef<const Symmetric::Options>> {
        auto *SymmetricOptions = std::get_if<Symmetric::Options>(&Options);
        if (!SymmetricOptions) {
          return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
        }
        return SymmetricOptions;
      });
  if (!OptSymmetricOptionsResult) {
    return WasiCryptoUnexpect(OptSymmetricOptionsResult);
  }

  return Symmetric::openState(Alg, asOptionalRef(*OptKeyResult),
                              *OptSymmetricOptionsResult)
      .and_then([this](auto &&State) noexcept {
        return SymmetricStateManager.registerManager(
            std::forward<decltype(State)>(State));
      });
}

WasiCryptoExpect<__wasi_symmetric_state_t>
Context::symmetricStateClone(__wasi_symmetric_state_t StateHandle) noexcept {
  return SymmetricStateManager.get(StateHandle)
      .and_then(&Symmetric::stateClone)
      .and_then([this](auto &&State) noexcept {
        return SymmetricStateManager.registerManager(
            std::forward<decltype(State)>(State));
      });
}

WasiCryptoExpect<__wasi_symmetric_key_t>
Context::symmetricKeyGenerateManaged(__wasi_secrets_manager_t,
                                     Symmetric::Algorithm,
                                     __wasi_opt_options_t) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<void> Context::symmetricKeyStoreManaged(
    __wasi_secrets_manager_t, __wasi_symmetric_key_t, Span<uint8_t>) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<__wasi_version_t>
Context::symmetricKeyReplaceManaged(__wasi_secrets_manager_t,
                                    __wasi_symmetric_key_t,
                                    __wasi_symmetric_key_t) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::tuple<size_t, __wasi_version_t>>
Context::symmetricKeyId(__wasi_symmetric_key_t, Span<uint8_t>) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<__wasi_symmetric_key_t>
Context::symmetricKeyFromId(__wasi_secrets_manager_t, Span<uint8_t>,
                            __wasi_version_t) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
