// SPDX-License-Identifier: Apache-2.0

#include "host/wasi/crypto/ctx.h"

namespace WasmEdge {
namespace Host {
namespace WASI {
namespace Crypto {
HandleMangers::HandleMangers() : ArrayOutputManger{0x00} {}

CryptoCtx::CryptoCtx() {}

WasiCryptoExpect<__wasi_size_t>
CryptoCtx::arrayOutputLen(__wasi_array_output_t ArrayOutputHandle) {
  auto ArrayOutput = Mangers.ArrayOutputManger.get(ArrayOutputHandle);
  return ArrayOutput->len();
}

WasiCryptoExpect<__wasi_size_t>
CryptoCtx::arrayOutputPull(__wasi_array_output_t ArrayOutputHandle,
                           uint8_t_ptr Buf, __wasi_size_t BufLen) {
  auto ArrayOutput = Mangers.ArrayOutputManger.get(ArrayOutputHandle);
  auto Len = ArrayOutput->pull(Span<uint8_t>{reinterpret_cast<uint8_t*>(Buf), BufLen});
  Mangers.ArrayOutputManger.close(ArrayOutputHandle);
  return Len;
}

} // namespace Crypto
} // namespace WASI
} // namespace Host
} // namespace WasmEdge