// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/ctx.h"
#include "common/span.h"
#include "host/wasi_crypto/error.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<size_t>
WasiCryptoContext::arrayOutputLen(__wasi_array_output_t ArrayOutputHandle) {
  auto ArrayOutput = ArrayOutputManger.get(ArrayOutputHandle);
  if (!ArrayOutput) {
    return WasiCryptoUnexpect(ArrayOutput);
  }

  return (*ArrayOutput)->len();
}

WasiCryptoExpect<size_t>
WasiCryptoContext::arrayOutputPull(__wasi_array_output_t ArrayOutputHandle,
                                   Span<uint8_t> Buf) {
  auto ArrayOutput = ArrayOutputManger.get(ArrayOutputHandle);

  auto [Size, AlreadyConsumed] = (*ArrayOutput)->pull(Buf);
  if (AlreadyConsumed) {
    ArrayOutputManger.close(ArrayOutputHandle);
  }
  return Size;
}

WasiCryptoExpect<__wasi_options_t>
WasiCryptoContext::optionsOpen(__wasi_algorithm_type_e_t AlgorithmType) {
  return OptionsManger.registerManger(Common::optionsOpen(AlgorithmType));
}

WasiCryptoExpect<void>
WasiCryptoContext::optionsClose(__wasi_options_t Handle) {
  return OptionsManger.close(Handle);
}

WasiCryptoExpect<void>
WasiCryptoContext::optionsSet(__wasi_options_t OptionsHandle,
                              std::string_view Name,
                              Span<uint8_t const> Value) {
  auto Options = OptionsManger.get(OptionsHandle);
  if (!Options) {
    return WasiCryptoUnexpect(Options);
  }

  return Common::optionsSet(*Options, Name, Value);
}

WasiCryptoExpect<void>
WasiCryptoContext::optionsSetU64(__wasi_options_t OptionsHandle,
                                 std::string_view Name, uint64_t Value) {
  auto Options = OptionsManger.get(OptionsHandle);
  if (!Options) {
    return WasiCryptoUnexpect(Options);
  }

  return Common::optionsSetU64(*Options, Name, Value);
}

WasiCryptoExpect<void> WasiCryptoContext::optionsSetGuestBuffer(
    __wasi_options_t OptionsHandle, std::string_view Name, Span<uint8_t> Buf) {
  auto Options = OptionsManger.get(OptionsHandle);
  if (!Options) {
    return WasiCryptoUnexpect(Options);
  }
  return Common::optionsSetGuestBuffer(*Options, Name, Buf);
}

WasiCryptoExpect<__wasi_secrets_manager_t>
WasiCryptoContext::secretsMangerOpen(std::optional<__wasi_options_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
}

WasiCryptoExpect<void>
WasiCryptoContext::secretsMangerClose(__wasi_secrets_manager_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
}

WasiCryptoExpect<void> WasiCryptoContext::secretsManagerInvalidate(
    __wasi_secrets_manager_t, Span<uint8_t const>, __wasi_version_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoContext::allocateArrayOutput(std::vector<uint8_t> &&Data) {
  return ArrayOutputManger.registerManger(
      std::make_shared<Common::ArrayOutput>(std::move(Data)));
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge