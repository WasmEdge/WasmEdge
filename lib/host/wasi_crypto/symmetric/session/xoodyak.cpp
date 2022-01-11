// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/session/xoodyak.h"
#include <openssl/rand.h>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

WasiCryptoExpect<std::unique_ptr<Key>>
XoodyakKeyBuilder::generate(std::shared_ptr<Options>) {
  auto Len = keyLen();
  if (!Len) {
    return WasiCryptoUnexpect(Len);
  }
  std::vector<uint8_t> Raw(*Len, 0);
  RAND_bytes(Raw.data(), Raw.size());

  return std::make_unique<Key>(Alg, std::move(Raw));
}

WasiCryptoExpect<std::unique_ptr<Key>>
XoodyakKeyBuilder::import(Span<uint8_t const> Raw) {
  return std::make_unique<Key>(Alg,
                               std::vector<uint8_t>{Raw.begin(), Raw.end()});
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
XoodyakState::open(SymmetricAlgorithm, std::shared_ptr<Key>,
                   std::shared_ptr<Options>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}
WasiCryptoExpect<void> XoodyakState::absorb(Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<void> XoodyakState::squeeze(Span<uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}
WasiCryptoExpect<__wasi_size_t>
XoodyakState::decryptDetached(Span<uint8_t>, Span<const uint8_t>,
                              Span<uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
