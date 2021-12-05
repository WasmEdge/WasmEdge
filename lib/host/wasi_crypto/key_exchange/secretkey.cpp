// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/secretkey.h"
#include "host/wasi_crypto/signature/secretkey.h"
#include "host/wasi_crypto/asymmetric_common/secretkey.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<std::vector<uint8_t>> KxSecretKeyBase::dh(KxPublicKey &) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<std::vector<uint8_t>>
KxSecretKeyBase::decapsulate(Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

KxSecretKey::KxSecretKey(std::unique_ptr<KxSecretKeyBase> Key)
    : Inner(std::make_shared<Mutex<std::unique_ptr<KxSecretKeyBase>>>(
          std::move(Key))) {}

KxAlgorithm KxSecretKey::alg() {
  return Inner->locked(
      [](std::unique_ptr<KxSecretKeyBase> &Key) { return Key->alg(); });
}
WasiCryptoExpect<__wasi_size_t> KxSecretKey::len() {
  return Inner->locked(
      [](std::unique_ptr<KxSecretKeyBase> &Key) { return Key->len(); });
}

WasiCryptoExpect<Span<const uint8_t>> KxSecretKey::asRaw() {
  return Inner->locked(
      [](std::unique_ptr<KxSecretKeyBase> &Key) { return Key->asRaw(); });
}

WasiCryptoExpect<std::vector<uint8_t>> KxSecretKey::dh(KxPublicKey &KxPk) {
  return Inner->locked(
      [&KxPk](std::unique_ptr<KxSecretKeyBase> &Key) { return Key->dh(KxPk); });
}

WasiCryptoExpect<std::vector<uint8_t>>
KxSecretKey::decapsulate(Span<const uint8_t> EncapsulatedSecret) {
  return Inner->locked([EncapsulatedSecret = std::move(EncapsulatedSecret)](
                           std::unique_ptr<KxSecretKeyBase> &Key) {
    return Key->decapsulate(EncapsulatedSecret);
  });
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
