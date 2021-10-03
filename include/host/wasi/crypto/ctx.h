// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi/crypto/array_output.h"
#include "host/wasi/crypto/handles.h"
#include "common/span.h"
#include "wasi/crypto/api.hpp"
#include <array>

namespace WasmEdge {
namespace Host {
namespace WASI {
namespace Crypto {

struct HandleMangers {
  HandleMangers();
  HandlesManger<ArrayOutput> ArrayOutputManger;
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

private:
  HandleMangers Mangers;
};

} // namespace Crypto
} // namespace WASI
} // namespace Host
} // namespace WasmEdge