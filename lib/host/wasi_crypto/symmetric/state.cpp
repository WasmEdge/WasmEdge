// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/state.h"
#include "host/wasi_crypto/symmetric/aeads/aes_gcm.h"
#include "host/wasi_crypto/symmetric/aeads/charcha_poly.h"
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
            std::shared_ptr<Option> OptOption) {
  // if opt key exist
  if (OptKey) {
    // ensure key's alg equal with input alg
    ensureOrReturn(
        (OptKey->inner().locked([](auto &Key) { return Key.Alg; }) != Alg),
        __WASI_CRYPTO_ERRNO_INVALID_KEY);
  }

  switch (Alg) {
  case SymmetricAlgorithm::HmacSha256:
  case SymmetricAlgorithm::HmacSha512:
    if (auto Res = HmacSha2State::open(Alg, OptKey, OptOption); !Res) {
      return WasiCryptoUnexpect(Res);
    } else {
      return std::move(*Res);
    }
  case SymmetricAlgorithm::HkdfSha256Extract:
  case SymmetricAlgorithm::HkdfSha512Extract:
  case SymmetricAlgorithm::HkdfSha256Expand:
  case SymmetricAlgorithm::HkdfSha512Expand:
    if (auto Res = HkdfState::open(Alg, OptKey, OptOption); !Res) {
      return WasiCryptoUnexpect(Res);
    } else {
      return std::move(*Res);
    }
  case SymmetricAlgorithm::Sha256:
  case SymmetricAlgorithm::Sha512:
  case SymmetricAlgorithm::Sha512_256:
    if (auto Res = Sha2State::open(Alg, OptKey, OptOption); !Res) {
      return WasiCryptoUnexpect(Res);
    } else {
      return std::move(*Res);
    }
  case SymmetricAlgorithm::Aes128Gcm:
  case SymmetricAlgorithm::Aes256Gcm:
    if (auto Res = AesGcmState::open(Alg, OptKey, OptOption); !Res) {
      return WasiCryptoUnexpect(Res);
    } else {
      return std::move(*Res);
    }
  case SymmetricAlgorithm::ChaCha20Poly1305:
  case SymmetricAlgorithm::XChaCha20Poly1305:
    if (auto Res = ChaChaPolyState::open(Alg, OptKey, OptOption); !Res) {
      return WasiCryptoUnexpect(Res);
    } else {
      return std::move(*Res);
    }
  case SymmetricAlgorithm::Xoodyak128:
  case SymmetricAlgorithm::Xoodyak160:
    if (auto Res = XoodyakState::open(Alg, OptKey, OptOption); !Res) {
      return WasiCryptoUnexpect(Res);
    } else {
      return std::move(*Res);
    }
  default:
    __builtin_unreachable();
  }
}

WasiCryptoExpect<void> State::absorb(Span<const uint8_t> Data) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<void> State::squeeze(Span<uint8_t> Out) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<void> State::ratchet() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<__wasi_size_t> State::encrypt(Span<uint8_t> Out,
                                               Span<const uint8_t> Data) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<Tag> State::encryptDetached(Span<uint8_t> Out,
                                             Span<const uint8_t> Data) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<__wasi_size_t> State::decrypt(Span<uint8_t> Out,
                                               Span<const uint8_t> Data) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<__wasi_size_t> State::decryptDetached(Span<uint8_t> Out,
                                                       Span<const uint8_t> Data,
                                                       Span<uint8_t> RawTag) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<std::unique_ptr<Key>> State::squeezeKey(SymmetricAlgorithm KeyAlg) {
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
