// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/ctx.h"
#include "host/wasi_crypto/common/array_output.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

WasiCryptoExpect<size_t>
Context::arrayOutputLen(__wasi_array_output_t ArrayOutputHandle) noexcept {
  return ArrayOutputManger.get(ArrayOutputHandle)
      .map(&Common::ArrayOutput::len);
}

WasiCryptoExpect<size_t>
Context::arrayOutputPull(__wasi_array_output_t ArrayOutputHandle,
                         Span<uint8_t> Buf) noexcept {
  return ArrayOutputManger.get(ArrayOutputHandle)
      .map([=](Common::ArrayOutput &ArrayOutput) noexcept {
        auto [Size, AlreadyConsumed] = ArrayOutput.pull(Buf);
        if (AlreadyConsumed) {
          ArrayOutputManger.close(ArrayOutputHandle);
        }
        return Size;
      });
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge