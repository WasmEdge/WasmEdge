// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/session/xoodyak.h"
#include "host/wasi_crypto/wrapper/random.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

XoodyakKeyBuilder::XoodyakKeyBuilder(SymmetricAlgorithm Alg)
    : Alg(Alg) {}

WasiCryptoExpect<Key>
XoodyakKeyBuilder::generate(std::shared_ptr<Options>) {
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

WasiCryptoExpect<Key>
XoodyakKeyBuilder::import(Span<uint8_t const> Raw) {
//  return Key{std::make_unique<XoodyakKey>(Alg, Raw)};
}

WasiCryptoExpect<__wasi_size_t> XoodyakKeyBuilder::keyLen() {
  switch (Alg) {
  case SymmetricAlgorithm::Xoodyak128:
    return 16;
  case SymmetricAlgorithm::Xoodyak160:
    return 20;
  default:
    return __WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM;
  }
}

WasiCryptoExpect<std::unique_ptr<XoodyakState>>
XoodyakState::import(SymmetricAlgorithm, std::optional<Key>,
                              std::shared_ptr<Options>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
