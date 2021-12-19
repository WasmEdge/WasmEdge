// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/wrapper/aes_gcm.h"
#include "host/wasi_crypto/symmetric/tag.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<AesGcmCtx> AesGcmCtx::import(SymmetricAlgorithm Alg,
                                              Span<uint8_t const> Key,
                                              Span<uint8_t const> Nonce) {
  // Init unique_ptr
  OpenSSLUniquePtr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free> Ctx{
      EVP_CIPHER_CTX_new()};

  EVP_CIPHER const *Cipher;
  switch (Alg) {
  case SymmetricAlgorithm::Aes128Gcm:
    Cipher = EVP_aes_128_gcm();
    break;
  case SymmetricAlgorithm::Aes256Gcm:
    Cipher = EVP_aes_256_gcm();
    break;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  }

  if (Nonce.size() != NonceLen) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
  }

  if (1 != EVP_CipherInit_ex(Ctx.get(), Cipher, nullptr, Key.data(),
                             Nonce.data(), Mode::Unchanged)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return AesGcmCtx{Alg, std::move(Ctx)};
}

WasiCryptoExpect<void> AesGcmCtx::absorb(Span<const uint8_t> Data) {
  int Len;
  // TODO: need change Openssl AAD default length from 12 if beyond?
  if (1 !=
      EVP_CipherUpdate(Ctx.get(), nullptr, &Len, Data.data(), Data.size())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return {};
}

WasiCryptoExpect<std::vector<uint8_t>>
AesGcmCtx::encryptDetached(Span<uint8_t> Out, Span<const uint8_t> Data) {
  if (1 != EVP_CipherInit_ex(Ctx.get(), nullptr, nullptr, nullptr, nullptr,
                             Mode::Encrypt)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  //  auto Nonce = Options.get("nonce");
  //  if (!Nonce) {
  //    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NONCE_REQUIRED);
  //  }

  //  if (Out.data() != Data.data()) {
  //    std::copy(Data.begin(), Data.end(), Out.begin());
  //  }

  int ActualOutSize;
  if (1 != EVP_CipherUpdate(Ctx.get(), Out.data(), &ActualOutSize, Data.data(),
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
  // However, cannot do put nullptr length in it. construct a temp var
  // TODO:Better
  int AL;
  if (1 != EVP_CipherFinal_ex(Ctx.get(), nullptr, &AL)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  // Gen tag
  std::vector<uint8_t> RawTagData;
  RawTagData.reserve(TagLen);
  RawTagData.resize(TagLen);

  if (1 != EVP_CIPHER_CTX_ctrl(Ctx.get(), EVP_CTRL_GCM_GET_TAG, TagLen,
                               RawTagData.data())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return RawTagData;
}

WasiCryptoExpect<__wasi_size_t>
AesGcmCtx::decryptDetached(Span<uint8_t> Out, Span<const uint8_t> Data,
                           Span<const uint8_t> RawTag) {
  if (1 != EVP_CipherInit_ex(Ctx.get(), nullptr, nullptr, nullptr, nullptr,
                             Mode::Decrypt)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  int ActualOutSize;
  if (1 != EVP_CipherUpdate(Ctx.get(), Out.data(), &ActualOutSize, Data.data(),
                            Data.size())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_TAG);
  }

  if (1 != EVP_CIPHER_CTX_ctrl(Ctx.get(), EVP_CTRL_GCM_SET_TAG, TagLen,
                           const_cast<uint8_t *>(RawTag.data()))) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  // Notice: Finalise the decryption. Normally ciphertext bytes may be written
  // at this stage, but this does not occur in GCM mode
  // However, cannot do put nullptr length in it. construct a temp var
  // TODO:Better
  int AL;
  if (1 != EVP_CipherFinal_ex(Ctx.get(), nullptr, &AL)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return ActualOutSize;
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
