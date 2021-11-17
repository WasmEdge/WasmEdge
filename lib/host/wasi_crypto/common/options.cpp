// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/common/options.h"
#include "host/wasi_crypto/key_exchange/options.h"
#include "host/wasi_crypto/signature/options.h"
#include "host/wasi_crypto/symmetric/options.h"

#include <variant>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<Options> Options::make(__wasi_algorithm_type_e_t Algorithm) {
  switch (Algorithm) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES:
    return Options{SignatureOptions{}};
  case __WASI_ALGORITHM_TYPE_SYMMETRIC:
    return Options{SymmetricOptions{}};
  case __WASI_ALGORITHM_TYPE_KEY_EXCHANGE:
    return Options{KxOptions{}};
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  }
}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge