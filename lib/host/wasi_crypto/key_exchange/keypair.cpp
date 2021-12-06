// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/keypair.h"
#include "host/wasi_crypto/key_exchange/dh/x25519.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<std::vector<uint8_t>> KxKeyPair::Base::asRaw() {
  auto Pk = publicKey();
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  auto PkRaw =
      Pk->inner()->locked([](auto &PkInner) { return PkInner->asRaw(); });
  if (!PkRaw) {
    return WasiCryptoUnexpect(PkRaw);
  }

  auto Sk = secretKey();
  if (!Sk) {
    return WasiCryptoUnexpect(Sk);
  }

  auto SkRaw =
      Sk->inner()->locked([](auto &SkInner) { return SkInner->asRaw(); });
  if (!SkRaw) {
    return WasiCryptoUnexpect(SkRaw);
  }

  PkRaw->insert(PkRaw->end(), SkRaw->begin(), SkRaw->end());

  return PkRaw;
}

WasiCryptoExpect<std::vector<uint8_t>>
KxKeyPair::Base::exportData(__wasi_keypair_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    auto Res = asRaw();
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }

    return std::vector<uint8_t>{Res->begin(), Res->end()};
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  }
}

WasiCryptoExpect<std::unique_ptr<KxKeyPair::Builder>>
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
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
