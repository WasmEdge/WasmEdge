// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/common/array_output.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Common {

WasiCryptoExpect<__wasi_size_t> ArrayOutput::pull(Span<uint8_t> Buf) {
  ensureOrReturn(Buf.size() >= Data.size(), __WASI_CRYPTO_ERRNO_OVERFLOW);

  std::copy(Data.begin(), Data.end(), Buf.begin());

  ensureOrReturn(Data.size() <= std::numeric_limits<int>::max(),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  return static_cast<__wasi_size_t>(Data.size());
}

WasiCryptoExpect<__wasi_size_t> ArrayOutput::len() {
  ensureOrReturn(Data.size() <= std::numeric_limits<int>::max(),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  return static_cast<__wasi_size_t>(Data.size());
}

} // namespace Common
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge