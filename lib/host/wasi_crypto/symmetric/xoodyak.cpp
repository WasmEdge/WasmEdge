// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/xoodyak.h"
#include "host/wasi_crypto/wrapper/random.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
XoodyakSymmetricKey::XoodyakSymmetricKey(SymmetricAlgorithm Alg,
                                         Span<uint8_t const> Raw)
    : Alg(Alg), Raw(Raw.begin(), Raw.end()) {}

XoodyakSymmetricKeyBuilder::XoodyakSymmetricKeyBuilder(SymmetricAlgorithm Alg)
    : Alg(Alg) {}

WasiCryptoExpect<SymmetricKey>
XoodyakSymmetricKeyBuilder::generate(
    std::optional<SymmetricOptions> ) {
  auto Len = keyLen();
  CryptoRandom Random;
  if (!Len) {
    return WasiCryptoUnexpect(Len);
  }
  std::vector<uint8_t> Raw(*Len, 0);
  if (auto Res = Random.fill(Raw); !Res.has_value()) {
    return WasiCryptoUnexpect(Res);
  }
  return import(Raw);
}

WasiCryptoExpect<SymmetricKey> XoodyakSymmetricKeyBuilder::import(Span<uint8_t const> Raw) {
  return SymmetricKey{std::make_unique<XoodyakSymmetricKey>(Alg, Raw)};
}

WasiCryptoExpect<__wasi_size_t> XoodyakSymmetricKeyBuilder::keyLen() {
  switch (Alg) {
  case SymmetricAlgorithm::Xoodyak128:
    return 16;
  case SymmetricAlgorithm::Xoodyak160:
    return 20;
  default:
    return __WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM;
  }
}

WasiCryptoExpect<std::unique_ptr<XoodyakSymmetricState>>
XoodyakSymmetricState::import(SymmetricAlgorithm, std::optional<SymmetricKey>,
                            std::optional<SymmetricOptions>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
