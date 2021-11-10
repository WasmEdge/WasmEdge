// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/common/array_output.h"
#include "host/wasi_crypto/common/options.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/handles.h"
#include "wasi_crypto/api.hpp"

#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class CommonContext {
public:
  /// Return the length of an `array_output` object.
  ///
  /// This allows a guest to allocate a buffer of the correct size in order to
  /// copy the output of a function returning this object type.
  WasiCryptoExpect<__wasi_size_t>
  arrayOutputLen(__wasi_array_output_t ArrayOutputHandle);

  /// Copy the content of an `array_output` object into an application-allocated
  /// buffer.
  ///
  /// Multiple calls to that function can be made in order to consume the data
  /// in a streaming fashion, if necessary.
  ///
  /// The function returns the number of bytes that were actually copied. `0`
  /// means that the end of the stream has been reached. The total size always
  /// matches the output of `array_output_len()`.
  ///
  /// The handle is automatically closed after all the data has been consumed.
  ///
  /// Example usage:
  ///
  /// ```rust
  /// let len = array_output_len(output_handle)?;
  /// let mut out = vec![0u8; len];
  /// array_output_pull(output_handle, &mut out)?;
  ///
  WasiCryptoExpect<__wasi_size_t>
  arrayOutputPull(__wasi_array_output_t ArrayOutputHandle,
                  Span<uint8_t> BufPtr);

  /// Create a new object to set non-default options.
  ///
  /// Example usage:
  ///
  /// ```rust
  /// let options_handle = options_open(AlgorithmType::Symmetric)?;
  /// options_set(options_handle, "context", context)?;
  /// options_set_u64(options_handle, "threads", 4)?;
  /// let state = symmetric_state_open("BLAKE3", None, Some(options_handle))?;
  /// options_close(options_handle)?;
  /// ```
  WasiCryptoExpect<__wasi_options_t>
  optionsOpen(__wasi_algorithm_type_e_t AlgorithmType);

  /// Destroy an options object.
  ///
  /// Objects are reference counted. It is safe to close an object immediately
  /// after the last function needing it is called.
  WasiCryptoExpect<void> optionsClose(__wasi_options_t Handle);

  /// Set or update an option.
  ///
  /// This is used to set algorithm-specific parameters.
  ///
  /// This function may return `unsupported_option` if an option that doesn't
  /// exist for any implemented algorithms is specified.
  WasiCryptoExpect<void> optionsSet(__wasi_options_t OptionsHandle,
                                    std::string_view Name,
                                    Span<uint8_t const> Value);

  /// Set or update an integer option.
  ///
  /// This is used to set algorithm-specific parameters.
  ///
  /// This function may return `unsupported_option` if an option that doesn't
  /// exist for any implemented algorithms is specified.
  WasiCryptoExpect<void> optionsSetU64(__wasi_options_t OptionsHandle,
                                       std::string_view Name, uint64_t Value);

  /// Set or update a guest-allocated memory that the host can use or return
  /// data into.
  ///
  /// This is for example used to set the scratch buffer required by memory-hard
  /// functions.
  ///
  /// This function may return `unsupported_option` if an option that doesn't
  /// exist for any implemented algorithms is specified.
  WasiCryptoExpect<void> optionsSetGuestBuffer(__wasi_options_t OptionsHandle,
                                               std::string_view Name,
                                               Span<uint8_t> Buf);
  /// __(optional)__
  /// Create a context to use a secrets manager.
  ///
  /// The set of required and supported options is defined by the host.
  ///
  /// The function returns the `unsupported_feature` error code if secrets
  /// management facilities are not supported by the host. This is also an
  /// optional import, meaning that the function may not even exist.
  WasiCryptoExpect<__wasi_secrets_manager_t>
  secretsMangerOpen(std::optional<__wasi_options_t> Options);

  /// __(optional)__
  /// Destroy a secrets manager context.
  ///
  /// The function returns the `unsupported_feature` error code if secrets
  /// management facilities are not supported by the host. This is also an
  /// optional import, meaning that the function may not even exist.
  WasiCryptoExpect<void>
  secretsMangerClose(__wasi_secrets_manager_t SecretsManger);

  WasiCryptoExpect<void>
  secretsManagerInvalidate(__wasi_secrets_manager_t SecretsManger,
                           Span<uint8_t const> KeyId, __wasi_version_t Version);

private:
  // TODO: I don't want to integrate the context of all modules, but this solution will lead to coupling.
  friend class SymmetricContext;

  WasiCryptoExpect<uint8_t> allocateArrayOutput(Span<uint8_t> Data);

  WasiCryptoExpect<std::shared_ptr<OptionBase>>
  readOption(__wasi_options_t OptionsHandle);

  HandlesManger<__wasi_array_output_t, ArrayOutput> ArrayOutputManger{0x00};
  HandlesManger<__wasi_options_t, std::shared_ptr<OptionBase>> OptionsManger{
      0x01};
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge