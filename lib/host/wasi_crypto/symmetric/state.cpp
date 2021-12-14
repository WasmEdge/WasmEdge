// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/state.h"
#include "host/wasi_crypto/symmetric/aes_gcm.h"
#include "host/wasi_crypto/symmetric/charcha_poly.h"
#include "host/wasi_crypto/symmetric/hkdf.h"
#include "host/wasi_crypto/symmetric/hmac_sha2.h"
#include "host/wasi_crypto/symmetric/sha2.h"
#include "host/wasi_crypto/symmetric/xoodyak.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

namespace {
constexpr bool checkAdd(Span<const uint8_t> Data, __wasi_size_t TagSize) {
  return (SIZE_MAX - TagSize) < Data.size();
}

constexpr bool checkSub(Span<const uint8_t> Data, __wasi_size_t TagSize) {
  return TagSize > Data.size();
}

} // namespace

WasiCryptoExpect<void> SymmetricState::Base::absorb(Span<uint8_t const>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<void> SymmetricState::Base::squeeze(Span<uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<void> SymmetricState::Base::ratchet() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<__wasi_size_t>
SymmetricState::Base::encrypt(Span<uint8_t> Out, Span<const uint8_t> Data) {
  auto TagSize = maxTagLen();
  if (!TagSize) {
    return WasiCryptoUnexpect(TagSize);
  }

  if (checkAdd(Data, *TagSize)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OVERFLOW);
  }
  if (Out.size() != Data.size() + *TagSize) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_LENGTH);
  }

  return encryptUnchecked(Out, Data);
}

WasiCryptoExpect<SymmetricTag>
SymmetricState::Base::encryptDetached(Span<uint8_t> Out,
                                      Span<const uint8_t> Data) {
  if (Out.size() != Data.size())
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_LENGTH);
  return encryptDetachedUnchecked(Out, Data);
}

WasiCryptoExpect<__wasi_size_t>
SymmetricState::Base::decrypt(Span<uint8_t> Out, Span<const uint8_t> Data) {
  if (auto TagSize = maxTagLen(); !TagSize) {
    return WasiCryptoUnexpect(TagSize);
  } else {
    if (checkSub(Data, *TagSize))
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OVERFLOW);
    if (Out.size() != Data.size() - *TagSize)
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OVERFLOW);
  }

  auto Result = decryptUnchecked(Out, Data);
  if (!Result) {
    std::fill(Out.begin(), Out.end(), 0);
    return WasiCryptoUnexpect(Result);
  }

  return *Result;
}

WasiCryptoExpect<__wasi_size_t> SymmetricState::Base::decryptDetached(
    Span<uint8_t> Out, Span<const uint8_t> Data, Span<uint8_t> RawTag) {

  if (Out.size() != Data.size()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_LENGTH);
  }

  auto Result = decryptDetachedUnchecked(Out, Data, RawTag);
  if (!Result) {
    std::fill(Out.begin(), Out.end(), 0);
    return WasiCryptoUnexpect(Result);
  }

  return *Result;
}

WasiCryptoExpect<SymmetricKey>
SymmetricState::Base::squeezeKey(SymmetricAlgorithm) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<SymmetricTag> SymmetricState::Base::squeezeTag() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<__wasi_size_t> SymmetricState::Base::maxTagLen() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<__wasi_size_t>
SymmetricState::Base::encryptUnchecked(Span<uint8_t>, Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<SymmetricTag>
SymmetricState::Base::encryptDetachedUnchecked(Span<uint8_t>,
                                               Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<__wasi_size_t>
SymmetricState::Base::decryptUnchecked(Span<uint8_t>, Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<__wasi_size_t> SymmetricState::Base::decryptDetachedUnchecked(
    Span<uint8_t>, Span<const uint8_t>, Span<uint8_t const>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<SymmetricState>
SymmetricState::import(SymmetricAlgorithm Alg,
                       std::optional<SymmetricKey> OptKey,
                       std::optional<SymmetricOptions> OptOptions) {
  if (OptKey &&
      (OptKey->inner()->locked([](auto &Key) { return Key->alg(); }) != Alg)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }

  switch (Alg) {
  case SymmetricAlgorithm::HmacSha256:
  case SymmetricAlgorithm::HmacSha512:
    if (auto Res = HmacSha2SymmetricState::import(Alg, OptKey, OptOptions);
        !Res) {
      return WasiCryptoUnexpect(Res);
    } else {
      return SymmetricState{std::move(*Res)};
    }
  case SymmetricAlgorithm::HkdfSha256Extract:
  case SymmetricAlgorithm::HkdfSha512Extract:
  case SymmetricAlgorithm::HkdfSha256Expand:
  case SymmetricAlgorithm::HkdfSha512Expand:
    if (auto Res = HkdfSymmetricState::import(Alg, OptKey, OptOptions); !Res) {
      return WasiCryptoUnexpect(Res);
    } else {
      return SymmetricState{std::move(*Res)};
    }
  case SymmetricAlgorithm::Sha256:
  case SymmetricAlgorithm::Sha512:
  case SymmetricAlgorithm::Sha512_256:
    if (auto Res = Sha2SymmetricState::import(Alg, OptKey, OptOptions); !Res) {
      return WasiCryptoUnexpect(Res);
    } else {
      return SymmetricState{std::move(*Res)};
    }
  case SymmetricAlgorithm::Aes128Gcm:
  case SymmetricAlgorithm::Aes256Gcm:
    if (auto Res = AesGcmSymmetricState::import(Alg, OptKey, OptOptions); !Res) {
      return WasiCryptoUnexpect(Res);
    } else {
      return SymmetricState{std::move(*Res)};
    }
  case SymmetricAlgorithm::ChaCha20Poly1305:
  case SymmetricAlgorithm::XChaCha20Poly1305:
    if (auto Res = ChaChaPolySymmetricState::make(Alg, OptKey, OptOptions);
        !Res) {
      return WasiCryptoUnexpect(Res);
    } else {
      return SymmetricState{std::move(*Res)};
    }
  case SymmetricAlgorithm::Xoodyak128:
  case SymmetricAlgorithm::Xoodyak160:
    if (auto Res = XoodyakSymmetricState::import(Alg, OptKey, OptOptions);
        !Res) {
      return WasiCryptoUnexpect(Res);
    } else {
      return SymmetricState{std::move(*Res)};
    }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
  }
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
