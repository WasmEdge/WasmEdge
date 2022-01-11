// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/aeads/chacha_poly.h"

#include <map>
#include <openssl/rand.h>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

// 128 size
inline __wasi_size_t TagLen = 16;

template <int NonceBit>
WasiCryptoExpect<std::unique_ptr<typename ChaChaPoly<NonceBit>::State>>
ChaChaPoly<NonceBit>::State::open(std::shared_ptr<Key> OptKey,
                                  std::shared_ptr<Options> OptOption) {
  ensureOrReturn(OptKey, __WASI_CRYPTO_ERRNO_KEY_REQUIRED);
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_NONCE_REQUIRED);

  auto Nonce = OptOption->get("nonce");
  if (!Nonce) {
    return WasiCryptoUnexpect(Nonce);
  }

  std::vector<uint8_t> Key =
      OptKey->inner().locked([](auto &Inner) { return Inner.Data; });

  ensureOrReturn(Nonce->size() == NonceBit / 8,
                 __WASI_CRYPTO_ERRNO_INVALID_HANDLE);
  //  ensureOrReturn(getKeySize(Alg) == Key.size(),
  //                 __WASI_CRYPTO_ERRNO_INVALID_HANDLE);

  EVP_CIPHER_CTX *Ctx = EVP_CIPHER_CTX_new();
  EVP_CIPHER_CTX_ctrl(Ctx, EVP_CTRL_AEAD_SET_IVLEN, NonceBit / 8, nullptr);

  opensslAssuming(Ctx);
  opensslAssuming(EVP_CipherInit_ex(Ctx, EVP_chacha20_poly1305(), nullptr,
                                    Key.data(), Nonce->data(),
                                    Mode::Unchanged));

  return std::make_unique<ChaChaPoly<NonceBit>::State>(Ctx, OptOption);
}

template <int NonceBit>
WasiCryptoExpect<std::unique_ptr<Key>>
ChaChaPoly<NonceBit>::KeyBuilder::generate(std::shared_ptr<Options>) {
  auto Len = keyLen();
  if (!Len) {
    return WasiCryptoUnexpect(Len);
  }

  std::vector<uint8_t> Res(*Len, 0);
  ensureOrReturn(RAND_bytes(Res.data(), Res.size()),
                 __WASI_CRYPTO_ERRNO_RNG_ERROR);

  return std::make_unique<Key>(Alg, std::move(Res));
}

template <int NonceBit>
WasiCryptoExpect<std::unique_ptr<Key>>
ChaChaPoly<NonceBit>::KeyBuilder::import(Span<uint8_t const> Raw) {
  return std::make_unique<Key>(Alg,
                               std::vector<uint8_t>{Raw.begin(), Raw.end()});
}

template <int NonceBit>
WasiCryptoExpect<__wasi_size_t> ChaChaPoly<NonceBit>::KeyBuilder::keyLen() {
  switch (Alg) {
  case SymmetricAlgorithm::ChaCha20Poly1305:
    return 16;
  case SymmetricAlgorithm::XChaCha20Poly1305:
    return 32;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
  }
}

template <int NonceBit>
WasiCryptoExpect<std::vector<uint8_t>>
ChaChaPoly<NonceBit>::State::optionsGet(std::string_view Name) {
  return OptOptions->get(Name);
}

template <int NonceBit>
WasiCryptoExpect<uint64_t>
ChaChaPoly<NonceBit>::State::optionsGetU64(std::string_view Name) {
  return OptOptions->getU64(Name);
}

template <int NonceBit>
WasiCryptoExpect<void>
ChaChaPoly<NonceBit>::State::absorb(Span<const uint8_t> Data) {
  int Len;
  opensslAssuming(
      EVP_CipherUpdate(Ctx.get(), nullptr, &Len, Data.data(), Data.size()));

  return {};
}

template <int NonceBit>
WasiCryptoExpect<Tag> ChaChaPoly<NonceBit>::State::encryptDetachedUnchecked(
    Span<uint8_t> Out, Span<const uint8_t> Data) {
  opensslAssuming(EVP_CipherInit_ex(Ctx.get(), nullptr, nullptr, nullptr,
                                    nullptr, Mode::Encrypt));
  //  auto Nonce = Options.get("nonce");
  //  if (!Nonce) {
  //    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NONCE_REQUIRED);
  //  }

  //  if (Out.data() != Data.data()) {
  //    std::copy(Data.begin(), Data.end(), Out.begin());
  //  }

  int ActualOutSize;
  opensslAssuming(EVP_CipherUpdate(Ctx.get(), Out.data(), &ActualOutSize,
                                   Data.data(), Data.size()));

  // we need check the equal.
  if (ActualOutSize < 0 ||
      static_cast<__wasi_size_t>(ActualOutSize) != Out.size()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_NONCE);
  }

  // Notice: Finalise the encryption. Normally ciphertext bytes may be written
  // at this stage, but this does not occur in GCM mode
  // However, cannot do put nullptr length in it. construct a temp var
  // TODO:Better
  int AL;
  opensslAssuming(EVP_CipherFinal_ex(Ctx.get(), nullptr, &AL));

  // Gen tag
  std::vector<uint8_t> RawTagData(TagLen);

  opensslAssuming(EVP_CIPHER_CTX_ctrl(Ctx.get(), EVP_CTRL_AEAD_GET_TAG, TagLen,
                                      RawTagData.data()));

  return RawTagData;
}

template <int NonceBit>
WasiCryptoExpect<__wasi_size_t>
ChaChaPoly<NonceBit>::State::decryptDetachedUnchecked(
    Span<uint8_t> Out, Span<const uint8_t> Data, Span<uint8_t const> RawTag) {
  opensslAssuming(EVP_CipherInit_ex(Ctx.get(), nullptr, nullptr, nullptr,
                                    nullptr, Mode::Decrypt));

  int ActualOutSize;
  opensslAssuming(EVP_CipherUpdate(Ctx.get(), Out.data(), &ActualOutSize,
                                   Data.data(), Data.size()));

  opensslAssuming(EVP_CIPHER_CTX_ctrl(Ctx.get(), EVP_CTRL_AEAD_SET_TAG, TagLen,
                                      const_cast<uint8_t *>(RawTag.data())));

  // Notice: Finalise the decryption. Normally ciphertext bytes may be written
  // at this stage, but this does not occur in GCM mode
  // However, cannot do put nullptr length in it. construct a temp var
  // TODO:Better
  int AL;
  opensslAssuming(EVP_CipherFinal_ex(Ctx.get(), Out.data(), &AL));

  return ActualOutSize;
}

template <int NonceBit>
WasiCryptoExpect<__wasi_size_t> ChaChaPoly<NonceBit>::State::maxTagLen() {
  return TagLen;
}

template class ChaChaPoly<96>;
// template class ChaChaPoly<192>;
} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
