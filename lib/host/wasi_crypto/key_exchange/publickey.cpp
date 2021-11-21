// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/publickey.h"
#include "host/wasi_crypto/key_exchange/dh/x25519.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<void> KxPublicKeyBase::verify() { return {}; }

WasiCryptoExpect<EncapsulatedSecret> KxPublicKeyBase::encapsulate() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

KxPublicKey::KxPublicKey(std::unique_ptr<KxPublicKeyBase> Key)
    : Inner(std::make_shared<Mutex<std::unique_ptr<KxPublicKeyBase>>>(
    std::move(Key))) {}

KxAlgorithm KxPublicKey::alg() {
  return Inner->locked(
      [](std::unique_ptr<KxPublicKeyBase> &Key) { return Key->alg(); });
}

WasiCryptoExpect<__wasi_size_t> KxPublicKey::len() {
  return Inner->locked(
      [](std::unique_ptr<KxPublicKeyBase> &Key) { return Key->len(); });
}

WasiCryptoExpect<std::vector<uint8_t>> KxPublicKey::asRaw() {
  return Inner->locked([](std::unique_ptr<KxPublicKeyBase> &Key)
                           -> WasiCryptoExpect<std::vector<uint8_t>> {
    auto Res = Key->asRaw();
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }
    return std::vector<uint8_t>{Res->begin(), Res->end()};
  });
}

WasiCryptoExpect<void> KxPublicKey::verify() {
  return Inner->locked(
      [](std::unique_ptr<KxPublicKeyBase> &Key) { return Key->verify(); });
}

WasiCryptoExpect<EncapsulatedSecret> KxPublicKey::encapsulate() {
  return Inner->locked([](std::unique_ptr<KxPublicKeyBase> &Key) {
    return Key->encapsulate();
  });
}
//WasiCryptoExpect<std::unique_ptr<KxPublicKeyBuilder>>
//KxPublicKey::builder(KxAlgorithm Alg) {
//  switch (Alg) {
//  case KxAlgorithm::X25519:
//    return std::make_unique<X25519PublicKey>();
//  case KxAlgorithm::Kyber768:
//    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
//  }
//}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
