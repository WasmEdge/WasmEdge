// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/charcha_poly.h"
#include "host/wasi_crypto/random.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

ChaChaPolySymmetricKey::ChaChaPolySymmetricKey(SymmetricAlgorithm Alg,
                                               Span<uint8_t const> Raw)
    : Alg(Alg), Raw(Raw.begin(), Raw.end()) {}

WasiCryptoExpect<Span<uint8_t>> ChaChaPolySymmetricKey::raw() { return Raw; }

SymmetricAlgorithm ChaChaPolySymmetricKey::alg() { return Alg; }

ChaChaPolySymmetricKeyBuilder::ChaChaPolySymmetricKeyBuilder(
    SymmetricAlgorithm Alg)
    : Alg{Alg} {}

WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
ChaChaPolySymmetricKeyBuilder::generate(std::shared_ptr<SymmetricOptions>) {
  auto Len = keyLen();
  CryptoRandom Random;
  if (!Len) {
    return WasiCryptoUnexpect(Len);
  }
  std::vector<uint8_t> Raw(*Len, 0);
  if(auto Res = Random.fill(Raw); !Res.has_value()) {
    return WasiCryptoUnexpect(Res);
  }

  return import(Raw);
}

WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
ChaChaPolySymmetricKeyBuilder::import(Span<uint8_t const> Raw) {
  return std::make_unique<ChaChaPolySymmetricKey>(Alg, Raw);
}

WasiCryptoExpect<__wasi_size_t> ChaChaPolySymmetricKeyBuilder::keyLen() {
  switch (Alg) {
  case SymmetricAlgorithm::ChaCha20Poly1305:
    return 16;
  case SymmetricAlgorithm::XChaCha20Poly1305:
    return 32;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
  }
}

WasiCryptoExpect<std::unique_ptr<ChaChaPolySymmetricState>>
ChaChaPolySymmetricState::make(SymmetricAlgorithm , std::shared_ptr<SymmetricKey> ,
                            std::shared_ptr<SymmetricOptions> ) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
