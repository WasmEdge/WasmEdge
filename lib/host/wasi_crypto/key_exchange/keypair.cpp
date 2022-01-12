// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/keypair.h"
#include "host/wasi_crypto/key_exchange/dh/x25519.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Kx {

WasiCryptoExpect<std::unique_ptr<KeyPair::Builder>>
KeyPair::Builder::builder(KxAlgorithm Alg) {
  switch (Alg) {
  case KxAlgorithm::X25519:
    return std::make_unique<X25519KeyPair::Builder>();
  case KxAlgorithm::Kyber768:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  default:
    assumingUnreachable();
  }
}

WasiCryptoExpect<std::unique_ptr<KeyPair>>
KeyPair::generate(KxAlgorithm Alg, std::shared_ptr<Options> Options) {
  auto Builder = Builder::builder(Alg);
  if (!Builder) {
    return WasiCryptoUnexpect(Builder);
  }

  return (*Builder)->generate(Options);
}

WasiCryptoExpect<std::unique_ptr<KeyPair>>
KeyPair::import(KxAlgorithm Alg, Span<uint8_t const> Raw,
                __wasi_keypair_encoding_e_t Encoding) {
  auto Builder = Builder::builder(Alg);
  if (!Builder) {
    return WasiCryptoUnexpect(Builder);
  }

  return (*Builder)->import(Raw, Encoding);
}

} // namespace Kx
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
