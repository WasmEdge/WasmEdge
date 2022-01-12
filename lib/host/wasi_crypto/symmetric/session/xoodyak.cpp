// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/session/xoodyak.h"
#include "openssl/rand.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

template <int KeyBits>
WasiCryptoExpect<std::unique_ptr<Key>>
Xoodyak<KeyBits>::KeyBuilder::generate(std::shared_ptr<Options>) {
  std::vector<uint8_t> Raw(keyLen(), 0);
  RAND_bytes(Raw.data(), Raw.size());

  return std::make_unique<Key>(Alg, std::move(Raw));
}

template <int KeyBits>
WasiCryptoExpect<std::unique_ptr<Key>>
Xoodyak<KeyBits>::KeyBuilder::import(Span<uint8_t const> Raw) {
  return std::make_unique<Key>(Alg,
                               std::vector<uint8_t>{Raw.begin(), Raw.end()});
}

template <int KeyBits> __wasi_size_t Xoodyak<KeyBits>::KeyBuilder::keyLen() {
  return KeyBits / 8;
}

template <int KeyBits>
WasiCryptoExpect<std::unique_ptr<typename Xoodyak<KeyBits>::State>>
Xoodyak<KeyBits>::State::open(SymmetricAlgorithm, std::shared_ptr<Key>,
                              std::shared_ptr<Options>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}
template <int KeyBits>
WasiCryptoExpect<void> Xoodyak<KeyBits>::State::absorb(Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int KeyBits>
WasiCryptoExpect<void> Xoodyak<KeyBits>::State::squeeze(Span<uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int KeyBits>
WasiCryptoExpect<__wasi_size_t>
Xoodyak<KeyBits>::State::decryptDetached(Span<uint8_t>, Span<const uint8_t>,
                                         Span<uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
