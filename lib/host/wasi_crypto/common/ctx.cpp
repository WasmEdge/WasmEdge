// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/common/ctx.h"
#include "common/span.h"
#include "host/wasi_crypto/asymmetric_common/ctx.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<__wasi_size_t>
CommonContext::arrayOutputLen(__wasi_array_output_t ArrayOutputHandle) {
  auto ArrayOutput = ArrayOutputManger.get(ArrayOutputHandle);
  return ArrayOutput->len();
}

WasiCryptoExpect<__wasi_size_t>
CommonContext::arrayOutputPull(__wasi_array_output_t ArrayOutputHandle,
                               Span<uint8_t> Buf) {
  auto ArrayOutput = ArrayOutputManger.get(ArrayOutputHandle);
  ArrayOutputManger.close(ArrayOutputHandle);
  return ArrayOutput->pull(Buf);
}

WasiCryptoExpect<__wasi_options_t>
CommonContext::optionsOpen(__wasi_algorithm_type_e_t AlgorithmType) {
  auto Options = OptionBase::make(AlgorithmType);

  if (!Options) {
    return WasiCryptoUnexpect(Options);
  }

  return OptionsManger.registerManger(std::move(*Options));
}

WasiCryptoExpect<void> CommonContext::optionsClose(__wasi_options_t Handle) {
  return OptionsManger.close(Handle);
}

WasiCryptoExpect<void> CommonContext::optionsSet(__wasi_options_t OptionsHandle,
                                                 std::string_view Name,
                                                 Span<uint8_t const> Value) {
  auto Options = OptionsManger.get(OptionsHandle);
  if (!Options) {
    return WasiCryptoUnexpect(Options);
  }

  return (*Options)->set(Name, Value);
}

WasiCryptoExpect<void>
CommonContext::optionsSetU64(__wasi_options_t OptionsHandle,
                             std::string_view Name, uint64_t Value) {
  auto Options = OptionsManger.get(OptionsHandle);
  if (!Options) {
    return WasiCryptoUnexpect(Options);
  }

  return (*Options)->setU64(Name, Value);
}

WasiCryptoExpect<void>
CommonContext::optionsSetGuestBuffer(__wasi_options_t OptionsHandle,
                                     std::string_view Name, Span<uint8_t> Buf) {
  auto Options = OptionsManger.get(OptionsHandle);
  if (!Options) {
    return WasiCryptoUnexpect(Options);
  }
  return (*Options)->setGuestBuffer(Name, Buf);
}

WasiCryptoExpect<__wasi_secrets_manager_t>
CommonContext::secretsMangerOpen(std::optional<__wasi_options_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
}

WasiCryptoExpect<void>
CommonContext::secretsMangerClose(__wasi_secrets_manager_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
}

WasiCryptoExpect<void>
CommonContext::secretsManagerInvalidate(__wasi_secrets_manager_t,
                                        Span<uint8_t const>, __wasi_version_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
}

WasiCryptoExpect<uint8_t>
CommonContext::allocateArrayOutput(Span<uint8_t> Data) {
  auto Output = ArrayOutput{Data};
  return ArrayOutputManger.registerManger(Output);
}

WasiCryptoExpect<std::shared_ptr<OptionBase>>
CommonContext::readOption(__wasi_options_t OptionsHandle) {
  auto Options = OptionsManger.get(OptionsHandle);
  if (!Options) {
    return WasiCryptoUnexpect(Options);
  }

  return *Options;
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge