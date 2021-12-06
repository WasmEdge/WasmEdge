// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/secretkey.h"
#include "host/wasi_crypto/signature/secretkey.h"
#include "host/wasi_crypto/asymmetric_common/secretkey.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<std::vector<uint8_t>> KxSecretKey::Base::dh(KxPublicKey &) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<std::vector<uint8_t>>
KxSecretKey::Base::decapsulate(Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

KxSecretKey::KxSecretKey(std::unique_ptr<Base> Key)
    : Inner(std::make_shared<Mutex<std::unique_ptr<Base>>>(
          std::move(Key))) {}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
