// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/tag.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/util.h"
#include "wasi_crypto/api.hpp"

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

WasiCryptoExpect<size_t> Tag::pull(Span<uint8_t> Raw) const {
  if (Raw.size() > Data.size()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_LENGTH);
  }
  if (Raw.size() < Data.size()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OVERFLOW);
  }

  std::copy(Data.begin(), Data.end(), Raw.begin());
  return Data.size();
}

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
