// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/state.h"
#include "host/wasi_crypto/symmetric/aeads/aes_gcm.h"
#include "host/wasi_crypto/symmetric/aeads/chacha_poly.h"
#include "host/wasi_crypto/symmetric/extract_and_expand/hkdf.h"
#include "host/wasi_crypto/symmetric/hash/sha2.h"
#include "host/wasi_crypto/symmetric/mac/hmac_sha2.h"
#include "host/wasi_crypto/symmetric/session/xoodyak.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

WasiCryptoExpect<std::unique_ptr<State>>
State::open(SymmetricAlgorithm Alg, std::shared_ptr<Key> OptKey,
            std::shared_ptr<Options> OptOption) {
  // if opt key exist
  if (OptKey) {
    // ensure key's alg equal with input alg
    ensureOrReturn(
        OptKey->inner().locked([](auto &Inner) { return Inner.Alg; }) == Alg,
        __WASI_CRYPTO_ERRNO_INVALID_KEY);
  }
  switch (Alg) {
  case SymmetricAlgorithm::HmacSha256:
    return HmacSha256::State::open(OptKey, OptOption);
  case SymmetricAlgorithm::HmacSha512:
    return HmacSha512::State::open(OptKey, OptOption);
  case SymmetricAlgorithm::HkdfSha256Expand:
    return Hkdf256Expand::State::open(OptKey, OptOption);
  case SymmetricAlgorithm::HkdfSha256Extract:
    return Hkdf256Extract::State::open(OptKey, OptOption);
  case SymmetricAlgorithm::HkdfSha512Expand:
    return Hkdf512Expand::State::open(OptKey, OptOption);
  case SymmetricAlgorithm::HkdfSha512Extract:
    return Hkdf512Extract::State::open(OptKey, OptOption);
  case SymmetricAlgorithm::Sha256:
    return Sha256State::open(OptKey, OptOption);
  case SymmetricAlgorithm::Sha512:
    return Sha512State::open(OptKey, OptOption);
  case SymmetricAlgorithm::Sha512_256:
    return Sha512_256State::open(OptKey, OptOption);
  case SymmetricAlgorithm::Aes128Gcm:
    return AesGcm128::State::open(OptKey, OptOption);
  case SymmetricAlgorithm::Aes256Gcm:
    return AesGcm256::State::open(OptKey, OptOption);
  case SymmetricAlgorithm::ChaCha20Poly1305:
    return ChaChaPoly1305::State::open(OptKey, OptOption);
  case SymmetricAlgorithm::XChaCha20Poly1305:
  case SymmetricAlgorithm::Xoodyak128:
  case SymmetricAlgorithm::Xoodyak160:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  default:
    assumingUnreachable();
  }
}

WasiCryptoExpect<void> State::absorb(Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<void> State::squeeze(Span<uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<void> State::ratchet() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<__wasi_size_t> State::encrypt(Span<uint8_t>,
                                               Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<Tag> State::encryptDetached(Span<uint8_t>,
                                             Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<__wasi_size_t> State::decrypt(Span<uint8_t>,
                                               Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<__wasi_size_t>
State::decryptDetached(Span<uint8_t>, Span<const uint8_t>, Span<uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<std::unique_ptr<Key>> State::squeezeKey(SymmetricAlgorithm) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<Tag> State::squeezeTag() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<__wasi_size_t> State::maxTagLen() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}
} // namespace Symmetric

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
