// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/aeads/aes_gcm.h"
#include "openssl/rand.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {
namespace {
constexpr const EVP_CIPHER *getCipher(SymmetricAlgorithm Alg) {
  switch (Alg) {
  case SymmetricAlgorithm::Aes128Gcm:
    return EVP_aes_128_gcm();
  case SymmetricAlgorithm::Aes256Gcm:
    return EVP_aes_256_gcm();
  default:
    assumingUnreachable();
  }
}

constexpr __wasi_size_t getKeySize(SymmetricAlgorithm Alg) {
  switch (Alg) {
  case SymmetricAlgorithm::Aes128Gcm:
    return 16;
  case SymmetricAlgorithm::Aes256Gcm:
    return 32;
  default:
    assumingUnreachable();
  }
}

inline constexpr __wasi_size_t NonceSize = 12;
// inline constexpr __wasi_size_t TagLen = 16;

} // namespace

WasiCryptoExpect<std::unique_ptr<Key>>
AesGcmKeyBuilder::generate(std::shared_ptr<Option>) {
  auto Len = keyLen();
  if (!Len) {
    return WasiCryptoUnexpect(Len);
  }

  std::vector<uint8_t> Res(*Len, 0);
  ensureOrReturn(RAND_bytes(Res.data(), Res.size()),
                 __WASI_CRYPTO_ERRNO_RNG_ERROR);

  return std::make_unique<Key>(Alg, std::move(Res));
}

WasiCryptoExpect<std::unique_ptr<Key>>
AesGcmKeyBuilder::import(Span<uint8_t const> Raw) {
  return std::make_unique<Key>(Alg,
                               std::vector<uint8_t>{Raw.begin(), Raw.end()});
}

WasiCryptoExpect<__wasi_size_t> AesGcmKeyBuilder::keyLen() {
  switch (Alg) {
  case SymmetricAlgorithm::Aes128Gcm:
    return 16;
  case SymmetricAlgorithm::Aes256Gcm:
    return 32;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
  }
}

WasiCryptoExpect<std::unique_ptr<AesGcmState>>
AesGcmState::open(SymmetricAlgorithm Alg, std::shared_ptr<Key> OptKey,
                  std::shared_ptr<Option> OptOption) {
  ensureOrReturn(OptKey, __WASI_CRYPTO_ERRNO_KEY_REQUIRED);
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_NONCE_REQUIRED);

  auto Nonce = OptOption->get("nonce");
  if (!Nonce) {
    return WasiCryptoUnexpect(Nonce);
  }

  std::vector<uint8_t> Key =
      OptKey->inner().locked([](auto &Inner) { return Inner.Data; });

  ensureOrReturn(Nonce->size() == NonceSize,
                 __WASI_CRYPTO_ERRNO_INVALID_HANDLE);
  ensureOrReturn(getKeySize(Alg) == Key.size(),
                 __WASI_CRYPTO_ERRNO_INVALID_HANDLE);

  EVP_CIPHER_CTX *Ctx = EVP_CIPHER_CTX_new();
  opensslAssuming(Ctx);
  opensslAssuming(EVP_CipherInit(Ctx, getCipher(Alg), Key.data(), Nonce->data(),
                                 Mode::Unchanged));

  return std::make_unique<AesGcmState>(Ctx, OptOption);
}

WasiCryptoExpect<std::vector<uint8_t>>
AesGcmState::optionsGet(std::string_view Name) {
  return OptOption->get(Name);
}

WasiCryptoExpect<uint64_t> AesGcmState::optionsGetU64(std::string_view Name) {
  return OptOption->getU64(Name);
}

WasiCryptoExpect<void> AesGcmState::absorb(Span<const uint8_t> Data) {
  int Len;
  // TODO: need change Openssl AAD default length from 12 if beyond?

  opensslAssuming(
      EVP_CipherUpdate(Ctx.get(), nullptr, &Len, Data.data(), Data.size()));

  return {};
}

WasiCryptoExpect<Tag>
AesGcmState::encryptDetachedUnchecked(Span<uint8_t> Out,
                                      Span<const uint8_t> Data) {

  opensslAssuming(EVP_CipherInit_ex(Ctx.get(), nullptr, nullptr, nullptr,
                                    nullptr, Mode::Encrypt));
  //  auto Nonce = Option.get("nonce");
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
  std::vector<uint8_t> RawTagData;
  RawTagData.reserve(TagLen);
  RawTagData.resize(TagLen);

  opensslAssuming(EVP_CIPHER_CTX_ctrl(Ctx.get(), EVP_CTRL_AEAD_GET_TAG, TagLen,
                                      RawTagData.data()));

  return RawTagData;
}

WasiCryptoExpect<__wasi_size_t> AesGcmState::decryptDetachedUnchecked(
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
  opensslAssuming(EVP_CipherFinal_ex(Ctx.get(), nullptr, &AL));

  return ActualOutSize;
}

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
