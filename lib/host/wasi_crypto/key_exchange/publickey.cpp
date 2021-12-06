// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/publickey.h"
#include "host/wasi_crypto/key_exchange/dh/x25519.h"
#include "host/wasi_crypto/signature/publickey.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<void> KxPublicKey::Base::verify() { return {}; }

WasiCryptoExpect<EncapsulatedSecret> KxPublicKey::Base::encapsulate() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

KxPublicKey::KxPublicKey(std::unique_ptr<Base> Key)
    : Inner(std::make_shared<Mutex<std::unique_ptr<Base>>>(
    std::move(Key))) {}

WasiCryptoExpect<std::vector<uint8_t>> KxPublicKey::asRaw() {
  return Inner->locked([](std::unique_ptr<Base> &Key)
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
      [](std::unique_ptr<Base> &Key) { return Key->verify(); });
}

WasiCryptoExpect<std::vector<uint8_t>>
KxPublicKey::exportData(__wasi_publickey_encoding_e_t Encoding) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<std::vector<uint8_t>>();
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
