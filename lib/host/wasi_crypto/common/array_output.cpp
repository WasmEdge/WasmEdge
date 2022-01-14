// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/common/array_output.h"
#include "common/errcode.h"

#include <algorithm>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Common {

WasiCryptoExpect<__wasi_size_t> ArrayOutput::pull(Span<uint8_t> Buf) {
  ensureOrReturn(Buf.size() >= Data.size(), __WASI_CRYPTO_ERRNO_OVERFLOW);

  std::copy(Data.begin(), Data.end(), Buf.begin());

  assuming(Data.size() < UINT32_MAX);
  return static_cast<__wasi_size_t>(Data.size());
}

WasiCryptoExpect<__wasi_size_t> ArrayOutput::len() { return Data.size(); }

} // namespace Common
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge