// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/options.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

WasiCryptoExpect<void> Options::set(std::string_view, Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
}

WasiCryptoExpect<void> Options::setU64(std::string_view, uint64_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
}

WasiCryptoExpect<void> Options::setGuestBuffer(std::string_view,
                                               Span<uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
}

WasiCryptoExpect<std::vector<uint8_t>> Options::get(std::string_view) const {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
}

WasiCryptoExpect<uint64_t> Options::getU64(std::string_view) const {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
}

} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
