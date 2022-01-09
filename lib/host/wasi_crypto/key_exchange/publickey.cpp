// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/publickey.h"
#include "host/wasi_crypto/key_exchange/dh/x25519.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Kx {

WasiCryptoExpect<std::unique_ptr<PublicKey>>
PublicKey::import(KxAlgorithm Alg, Span<const uint8_t> Raw,
                  __wasi_publickey_encoding_e_t Encoding) {
  switch (Alg) {
  case KxAlgorithm::X25519:
    return X25519PublicKey::import(Alg, Raw, Encoding);
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
