// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/symmetric/tag.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

WasiCryptoExpect<void> Tag::verify(Span<const uint8_t> RawTag) const noexcept {
  ensureOrReturn(
      std::equal(RawTag.begin(), RawTag.end(), Data.begin(), Data.end()),
      __WASI_CRYPTO_ERRNO_INVALID_TAG);

  return {};
}

WasiCryptoExpect<size_t> Tag::pull(Span<uint8_t> Raw) const noexcept {
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
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge