// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/common/array_output.h"

#include <algorithm>
#include <climits>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Common {

WasiCryptoExpect<size_t> ArrayOutput::pull(Span<uint8_t> Buf) {
  ensureOrReturn(Buf.size() >= Data.size(), __WASI_CRYPTO_ERRNO_OVERFLOW);

  std::copy(Data.begin(), Data.end(), Buf.begin());

  return Data.size();
}

WasiCryptoExpect<size_t> ArrayOutput::len() { return Data.size(); }

} // namespace Common
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge