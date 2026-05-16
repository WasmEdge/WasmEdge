// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ctx.h"
#include "common/array_output.h"
#include "common/options.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

WasiCryptoExpect<size_t>
Context::arrayOutputLen(__wasi_array_output_t ArrayOutputHandle) noexcept {
  return ArrayOutputManager.get(ArrayOutputHandle)
      .map(&Common::ArrayOutput::len);
}

WasiCryptoExpect<size_t>
Context::arrayOutputPull(__wasi_array_output_t ArrayOutputHandle,
                         Span<uint8_t> Buf) noexcept {
  return ArrayOutputManager.get(ArrayOutputHandle)
      .map([=](Common::ArrayOutput &ArrayOutput) noexcept {
        auto [Size, AlreadyConsumed] = ArrayOutput.pull(Buf);
        if (AlreadyConsumed) {
          ArrayOutputManager.close(ArrayOutputHandle);
        }
        return Size;
      });
}

WasiCryptoExpect<__wasi_options_t>
Context::optionsOpen(__wasi_algorithm_type_e_t AlgType) noexcept {
  return OptionsManager.registerManager(Common::optionsOpen(AlgType));
}

WasiCryptoExpect<void>
Context::optionsClose(__wasi_options_t OptionsHandle) noexcept {
  return OptionsManager.close(OptionsHandle);
}

WasiCryptoExpect<void> Context::optionsSet(__wasi_options_t OptionsHandle,
                                           std::string_view Name,
                                           Span<const uint8_t> Value) noexcept {
  return OptionsManager.get(OptionsHandle)
      .and_then([Name, Value](auto &&Options) noexcept {
        return Common::optionsSet(Options, Name, Value);
      });
}

WasiCryptoExpect<void> Context::optionsSetU64(__wasi_options_t OptionsHandle,
                                              std::string_view Name,
                                              uint64_t Value) noexcept {
  return OptionsManager.get(OptionsHandle)
      .and_then([Name, Value](auto &&Options) noexcept {
        return Common::optionsSetU64(Options, Name, Value);
      });
}

WasiCryptoExpect<void>
Context::optionsSetGuestBuffer(__wasi_options_t OptionsHandle,
                               std::string_view Name,
                               Span<uint8_t> Buf) noexcept {
  return OptionsManager.get(OptionsHandle)
      .and_then([Name, Buf](auto &&Options) noexcept {
        return Common::optionsSetGuestBuffer(Options, Name, Buf);
      });
}

WasiCryptoExpect<__wasi_secrets_manager_t>
Context::secretsManagerOpen(__wasi_opt_options_t OptOptionsHandle) noexcept {
  auto OptOptionsResult = mapAndTransposeOptional(
      OptOptionsHandle, [this](__wasi_options_t OptionsHandle) noexcept {
        return OptionsManager.get(OptionsHandle);
      });
  if (!OptOptionsResult) {
    return WasiCryptoUnexpect(OptOptionsResult);
  }

  // Refer to OptOptionsResult if it's a SecretsManager::Options.
  auto OptSecretsManagerOptionsResult = transposeOptionalToRef(
      *OptOptionsResult,
      [](const auto &Options) noexcept
          -> WasiCryptoExpect<OptionalRef<const SecretsManager::Options>> {
        auto *SecretsManagerOptions =
            std::get_if<SecretsManager::Options>(&Options);
        if (!SecretsManagerOptions) {
          return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
        }
        return SecretsManagerOptions;
      });
  if (!OptSecretsManagerOptionsResult) {
    return WasiCryptoUnexpect(OptSecretsManagerOptionsResult);
  }
  // TODO: Complete secrets manager opening logic.
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<void>
Context::secretsManagerClose(__wasi_secrets_manager_t) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<void>
Context::secretsManagerInvalidate(__wasi_secrets_manager_t, Span<const uint8_t>,
                                  __wasi_version_t) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
