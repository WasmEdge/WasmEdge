// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/common/options.h"
#include "host/wasi_crypto/signature/options.h"
#include "host/wasi_crypto/key_exchange/options.h"
#include "host/wasi_crypto/symmetric/options.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<void> OptionBase::set(std::string_view, Span<uint8_t const>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
}

WasiCryptoExpect<void> OptionBase::setU64(std::string_view, uint64_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
}

WasiCryptoExpect<void> OptionBase::setGuestBuffer(std::string_view,
                                                  Span<uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
}

WasiCryptoExpect<std::vector<uint8_t>> OptionBase::get(std::string_view) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
}

WasiCryptoExpect<uint64_t> OptionBase::getU64(std::string_view) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
}

WasiCryptoExpect<std::unique_ptr<OptionBase>>
OptionBase::make(__wasi_algorithm_type_e_t Algorithm) {
  switch (Algorithm) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES:
    return std::make_unique<SignatureOptions>();
  case __WASI_ALGORITHM_TYPE_SYMMETRIC:
    return std::make_unique<SymmetricOption>();
  case __WASI_ALGORITHM_TYPE_KEY_EXCHANGE:
    return std::make_unique<KxOptions>();
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  }
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge