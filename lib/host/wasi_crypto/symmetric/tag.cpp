// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/tag.h"
#include "host/wasi_crypto/util.h"

#include <algorithm>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

WasiCryptoExpect<void> Tag::verify(Span<uint8_t const> RawTag) const {
  ensureOrReturn(
      std::equal(RawTag.begin(), RawTag.end(), Data.begin(), Data.end()),
      __WASI_CRYPTO_ERRNO_INVALID_TAG);

  return {};
}

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
