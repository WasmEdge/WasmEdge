// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/asymmetric_common/secretkey.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<SignatureSecretKey> SecretKey::asSignatureSecretKey() {
  auto *Result = std::get_if<SignatureSecretKey>(&Inner);
  if (Result == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
  }
  return std::move(*Result);
}

WasiCryptoExpect<KxSecretKey> SecretKey::asKxSecretKey() {
  auto *Result = std::get_if<KxSecretKey>(&Inner);
  if (Result == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
  }

  return std::move(*Result);
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
