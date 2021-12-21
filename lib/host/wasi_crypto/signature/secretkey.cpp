// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/secretkey.h"
#include "host/wasi_crypto/key_exchange/dh/x25519.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<KxSecretKey>
KxSecretKey::import(KxAlgorithm Alg, Span<const uint8_t> Encoded,
                    __wasi_secretkey_encoding_e_t Encoding) {
  auto Builder = builder(Alg);
  if (!Builder) {
    return WasiCryptoUnexpect(Builder);
  }

  return (*Builder)->import(Encoded, Encoding);
}

WasiCryptoExpect<std::unique_ptr<KxSecretKey::Builder>>
KxSecretKey::builder(KxAlgorithm Alg) {
  switch (Alg) {
  case KxAlgorithm::X25519:
    return std::make_unique<X25519SecretKey::Builder>(Alg);
  case KxAlgorithm::Kyber768:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  default:
    __builtin_unreachable();
  }
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
