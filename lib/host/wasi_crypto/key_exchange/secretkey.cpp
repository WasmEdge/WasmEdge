// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/secretkey.h"
#include "host/wasi_crypto/key_exchange/dh/x25519.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Kx {
WasiCryptoExpect<std::unique_ptr<SecretKey>>
SecretKey::import(KxAlgorithm Alg, Span<const uint8_t> Encoded,
                  __wasi_secretkey_encoding_e_t Encoding) {
  switch (Alg) {
  case KxAlgorithm::X25519:
    return X25519SecretKey::import(Encoded, Encoding);
  case KxAlgorithm::Kyber768:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  default:
    assumingUnreachable();
  }
}
} // namespace Kx
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
