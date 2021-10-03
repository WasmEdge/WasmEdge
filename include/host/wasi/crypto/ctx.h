// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi/crypto/array_output.h"
#include "host/wasi/crypto/error.h"
#include "host/wasi/crypto/handles.h"
#include "host/wasi/crypto/options.h"
#include "wasi/crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WASI {
namespace Crypto {

struct HandleMangers {
  HandleMangers();
  HandlesManger<ArrayOutput> ArrayOutputManger;
  HandlesManger<Options> OptionsManger;
};

class CryptoCtx {
public:
  CryptoCtx();

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
  arrayOutputPull(__wasi_array_output_t ArrayOutputHandle, uint8_t_ptr Buf,
                  __wasi_size_t BufLen);

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

  /// Destroy an options object.
  ///
  /// Objects are reference counted. It is safe to close an object immediately
  /// after the last function needing it is called.
  WasiCryptoExpect<void> optionsSet(__wasi_options_t Handle,
                                    const char* Name,
                                    const_uint8_t_ptr Value,
                                    __wasi_size_t ValueLen);

  /// Set or update an integer option.
  ///
  /// This is used to set algorithm-specific parameters.
  ///
  /// This function may return `unsupported_option` if an option that doesn't
  /// exist for any implemented algorithms is specified.
  WasiCryptoExpect<void> optionsSetU64(__wasi_options_t Handle,
                                       const char* Name, uint64_t Value);

  /// Set or update a guest-allocated memory that the host can use or return
  /// data into.
  ///
  /// This is for example used to set the scratch buffer required by memory-hard
  /// functions.
  ///
  /// This function may return `unsupported_option` if an option that doesn't
  /// exist for any implemented algorithms is specified.
  WasiCryptoExpect<void> optionsSetGuestBuffer(__wasi_options_t Handle,
                                               const char* Name,
                                               uint8_t_ptr Buffer,
                                               __wasi_size_t BufferLen);

private:
  HandleMangers Mangers;
};

} // namespace Crypto
} // namespace WASI
} // namespace Host
} // namespace WasmEdge