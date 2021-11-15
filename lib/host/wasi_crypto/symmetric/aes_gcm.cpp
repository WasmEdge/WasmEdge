// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/aes_gcm.h"
#include "host/wasi_crypto/random.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

AesGcmSymmetricKey::AesGcmSymmetricKey(SymmetricAlgorithm Alg,
                                       Span<uint8_t const> Raw)
    : Alg(Alg), Raw(Raw.begin(), Raw.end()) {}

WasiCryptoExpect<Span<uint8_t>> AesGcmSymmetricKey::raw() { return Raw; }

SymmetricAlgorithm AesGcmSymmetricKey::alg() { return Alg; }

AesGcmSymmetricKeyBuilder::AesGcmSymmetricKeyBuilder(SymmetricAlgorithm Alg)
    : Alg(Alg) {}

WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
AesGcmSymmetricKeyBuilder::generate(std::shared_ptr<SymmetricOptions> Options) {
  auto Len = keyLen();
  if (!Len) {
    return WasiCryptoUnexpect(Len);
  }

  // have Nonce
  auto OptionsInner = Options->Inner.lock();
  if (Options != nullptr && OptionsInner->Nonce.has_value()) {
    // but size not equal
    if (OptionsInner->Nonce->size() != *Len) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_NONCE);
    }

    return import(*OptionsInner->Nonce);
  }

  // generate random by host TODO: may I need to generate a option and register. Need read proposal more detailed.
  std::vector<uint8_t> Raw(*Len, 0);
  CryptoRandom Random;
  if (auto Res = Random.fill(Raw); !Res) {
    return WasiCryptoUnexpect(Res);
  }

  return import(Raw);
}

WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
AesGcmSymmetricKeyBuilder::import(Span<uint8_t const> Raw) {
  return std::make_unique<AesGcmSymmetricKey>(Alg, Raw);
}

WasiCryptoExpect<__wasi_size_t> AesGcmSymmetricKeyBuilder::keyLen() {
  switch (Alg) {
  case SymmetricAlgorithm::Aes128Gcm:
    return 16;
  case SymmetricAlgorithm::Aes256Gcm:
    return 32;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
  }
}

WasiCryptoExpect<std::unique_ptr<AesGcmSymmetricState>>
AesGcmSymmetricState::make(SymmetricAlgorithm Algorithm,
                           std::shared_ptr<SymmetricKey> Key,
                           std::shared_ptr<SymmetricOptions> Options) {
  if (Key == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_KEY_REQUIRED);
  }

  // init get Key data
  auto AesGcmKey = std::dynamic_pointer_cast<AesGcmSymmetricKey>(Key);
  if (AesGcmKey == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }
  auto RawKey = AesGcmKey->raw();
  if (!RawKey) {
    return WasiCryptoUnexpect(RawKey);
  }

  // Init unique_ptr
  OpenSSlUniquePtr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free> Ctx{
      EVP_CIPHER_CTX_new()};
  if (Ctx == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  // Init Cipher
  EVP_CIPHER const *Cipher;
  switch (Algorithm) {
  case SymmetricAlgorithm::Aes128Gcm:
    Cipher = EVP_aes_128_gcm();
    break;
  case SymmetricAlgorithm::Aes256Gcm:
    Cipher = EVP_aes_256_gcm();
    break;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  }

  // init Nonce
  auto OptionsInner = Options->Inner.lock();
  if (Options == nullptr || !OptionsInner->Nonce) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NONCE_REQUIRED);
  }

  auto &Nonce = OptionsInner->Nonce;
  if (!Nonce || Nonce->size() != NonceLen) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NONCE_REQUIRED);
  }

  if (!EVP_CipherInit_ex(Ctx.get(), Cipher, nullptr, RawKey->data(),
                         Nonce->data(), Mode::Unchanged)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  //  new ptr
  return std::unique_ptr<AesGcmSymmetricState>{
      new AesGcmSymmetricState{Algorithm, std::move(Options), std::move(Ctx)}};
}

WasiCryptoExpect<void> AesGcmSymmetricState::absorb(Span<const uint8_t> Data) {
  int Len;
  // TODO: need change Openssl AAD default length from 12 if beyond?
  if (!EVP_CipherUpdate(Ctx.get(), nullptr, &Len, Data.data(), Data.size())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return {};
}

WasiCryptoExpect<__wasi_size_t>
AesGcmSymmetricState::encryptUnchecked(Span<uint8_t> Out,
                                       Span<const uint8_t> Data) {
  updateMode(Mode::Encrypt);

  auto Tag = encryptDetachedUnchecked({Out.data(), Data.size()}, Data);
  if (!Tag) {
    return WasiCryptoUnexpect(Tag);
  }

  // Gen tag
  auto Inner = Tag->raw();
  std::copy(Inner.begin(), Inner.end(), Out.begin() + Data.size());

  return Out.size();
}

WasiCryptoExpect<SymmetricTag>
AesGcmSymmetricState::encryptDetachedUnchecked(Span<uint8_t> Out,
                                               Span<const uint8_t> Data) {
  updateMode(Mode::Encrypt);

  auto Nonce = Options->Inner.lock()->Nonce;
  if (!Nonce) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NONCE_REQUIRED);
  }

  //  if (Out.data() != Data.data()) {
  //    std::copy(Data.begin(), Data.end(), Out.begin());
  //  }

  int ActualOutSize;
  if (!EVP_CipherUpdate(Ctx.get(), Out.data(), &ActualOutSize, Data.data(),
                        Data.size())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  // we need check the equal.
  if (ActualOutSize < 0 ||
      static_cast<__wasi_size_t>(ActualOutSize) != Out.size()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_NONCE);
  }

  // Notice: Finalise the encryption. Normally ciphertext bytes may be written
  // at this stage, but this does not occur in GCM mode
  // However, cannot do put nullptr length in it. construct a temp var TODO:Better
  int AL;
  if (!EVP_CipherFinal_ex(Ctx.get(), nullptr, &AL)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  // Gen tag
  std::array<uint8_t, TagLen> RawTagData;
  if (!EVP_CIPHER_CTX_ctrl(Ctx.get(), EVP_CTRL_GCM_GET_TAG, TagLen,
                           RawTagData.data())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  SymmetricTag Tag{Alg, {RawTagData.data(), TagLen}};
  return Tag;
}

WasiCryptoExpect<__wasi_size_t>
AesGcmSymmetricState::decryptUnchecked(Span<uint8_t> Out,
                                       Span<uint8_t const> Data) {
  updateMode(Mode::Decrypt);

  Span<uint8_t const> RawTag{Data.data() + Out.size(),
                             Data.size() - Out.size()};

  return decryptDetachedUnchecked(Out, {Data.data(), Out.size()}, RawTag);
}

WasiCryptoExpect<__wasi_size_t> AesGcmSymmetricState::decryptDetachedUnchecked(
    Span<uint8_t> Out, Span<const uint8_t> Data, Span<uint8_t const> RawTag) {
  updateMode(Mode::Decrypt);

  int ActualOutSize;
  if (!EVP_CipherUpdate(Ctx.get(), Out.data(), &ActualOutSize, Data.data(),
                        Data.size())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_TAG);
  }

  if (!EVP_CIPHER_CTX_ctrl(Ctx.get(), EVP_CTRL_GCM_SET_TAG, TagLen,
                           const_cast<uint8_t *>(RawTag.data()))) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  // Notice: Finalise the decryption. Normally ciphertext bytes may be written
  // at this stage, but this does not occur in GCM mode
  // However, cannot do put nullptr length in it. construct a temp var TODO:Better
  int AL;
  if (!EVP_CipherFinal_ex(Ctx.get(), nullptr, &AL)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return ActualOutSize;
}

AesGcmSymmetricState::AesGcmSymmetricState(
    SymmetricAlgorithm Algorithm, std::shared_ptr<SymmetricOptions> Options,
    OpenSSlUniquePtr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free> Ctx)
    : SymmetricState(Algorithm, std::move(Options)), Ctx(std::move(Ctx)) {}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
