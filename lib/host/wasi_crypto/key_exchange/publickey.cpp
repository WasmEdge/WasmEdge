// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/publickey.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<void> KxPublicKey::Base::verify() { return {}; }

WasiCryptoExpect<EncapsulatedSecret> KxPublicKey::Base::encapsulate() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
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
