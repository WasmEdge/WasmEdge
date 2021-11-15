// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/asymmetric_common/keypair.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<SignatureKeyPair> KeyPair::asSignatureKeyPair() {
  auto *Result = std::get_if<SignatureKeyPair>(&Inner);
  if (Result == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
  }

  return std::move(*Result);
}
WasiCryptoExpect<KxKeyPair> KeyPair::asKxKeyPair() {
  auto *Result = std::get_if<KxKeyPair>(&Inner);
  if (Result == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
  }

  return std::move(*Result);
}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
