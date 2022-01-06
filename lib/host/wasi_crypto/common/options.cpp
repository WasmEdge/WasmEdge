// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/common/options.h"
#include "host/wasi_crypto/key_exchange/options.h"
#include "host/wasi_crypto/signature/options.h"
#include "host/wasi_crypto/symmetric/options.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Common {

std::unique_ptr<Options> Options::open(__wasi_algorithm_type_e_t Alg) {
  switch (Alg) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES:
    return std::make_unique<SignatureOptions>(Alg);
  case __WASI_ALGORITHM_TYPE_SYMMETRIC:
    return std::make_unique<Symmetric::Option>(Alg);
  case __WASI_ALGORITHM_TYPE_KEY_EXCHANGE:
    return std::make_unique<KxOptions>(Alg);
  default:
    __builtin_unreachable();
  }
}

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

} // namespace Common
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge