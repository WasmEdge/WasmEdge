// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/symmetric/tag.h"
#include "openssl/crypto.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

WasiCryptoExpect<void> Tag::verify(Span<const uint8_t> RawTag) const noexcept {
  ensureOrReturn(
      !CRYPTO_memcmp(RawTag.data(), Data.raw().data(), RawTag.size()),
      __WASI_CRYPTO_ERRNO_INVALID_TAG);

  return {};
}

WasiCryptoExpect<size_t> Tag::pull(Span<uint8_t> Raw) const noexcept {
  if (Raw.size() > Data.raw().size()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_LENGTH);
  }
  if (Raw.size() < Data.raw().size()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OVERFLOW);
  }

  std::copy(Data.raw().begin(), Data.raw().end(), Raw.begin());
  return Data.raw().size();
}

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge