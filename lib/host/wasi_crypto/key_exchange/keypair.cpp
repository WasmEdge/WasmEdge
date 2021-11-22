// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/keypair.h"
#include "host/wasi_crypto/key_exchange/dh/x25519.h"
#include "host/wasi_crypto/asymmetric_common/keypair.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
KxKeyPair::KxKeyPair(std::unique_ptr<KxKeyPairBase> Inner)
    : Inner(std::make_shared<Mutex<std::unique_ptr<KxKeyPairBase>>>(
          std::move(Inner))) {}

WasiCryptoExpect<std::unique_ptr<KxKeyPairBuilder>>
KxKeyPair::builder(KxAlgorithm Alg) {
  switch (Alg) {
  case KxAlgorithm::X25519:
    return std::make_unique<X25519KeyPairBuilder>(Alg);
  case KxAlgorithm::Kyber768:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  }
}

WasiCryptoExpect<KxKeyPair>
KxKeyPair::generate(KxAlgorithm Alg, std::optional<KxOptions> Options) {
  auto Builder = builder(Alg);
  if (!Builder) {
    return WasiCryptoUnexpect(Builder);
  }

  return (*Builder)->generate(Options);
}

WasiCryptoExpect<std::vector<uint8_t>>
KxKeyPair::exportData(__wasi_keypair_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW:
    return Inner->locked([](std::unique_ptr<KxKeyPairBase> &Kp)
                             -> WasiCryptoExpect<std::vector<uint8_t>> {
      auto Res = Kp->asRaw();
      if (!Res) {
        return WasiCryptoUnexpect(Res);
      }

      return std::vector<uint8_t>{Res->begin(), Res->end()};
    });
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  }
}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
