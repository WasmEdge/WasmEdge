// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/wrapper/charcha_poly.h"
#include <map>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace {
const std::map<SymmetricAlgorithm, size_t> NonceMap{
    {SymmetricAlgorithm::ChaCha20Poly1305, 12},
    {SymmetricAlgorithm::XChaCha20Poly1305 , 24}
};
}

WasiCryptoExpect<ChaChaPolyCtx>
ChaChaPolyCtx::import(SymmetricAlgorithm Alg, Span<uint8_t const> Key,
                      Span<uint8_t const> Nonce) {
  // Init unique_ptr
  OpenSSLUniquePtr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free> Ctx{
      EVP_CIPHER_CTX_new()};
  opensslAssuming(Ctx);

  size_t NonceLen = NonceMap.at(Alg);

  if (Nonce.size() != NonceLen) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
  }

  opensslAssuming(EVP_CipherInit_ex(Ctx.get(), EVP_chacha20_poly1305(), nullptr, Key.data(),
                                    Nonce.data(), Mode::Unchanged));

  return ChaChaPolyCtx{Alg, std::move(Ctx)};
}

WasiCryptoExpect<void> ChaChaPolyCtx::absorb(Span<const uint8_t> Data) {
  int Len;
  // TODO: need change Openssl AAD default length from 12 if beyond?
  opensslAssuming(
      EVP_CipherUpdate(Ctx.get(), nullptr, &Len, Data.data(), Data.size()));

  return {};
}

WasiCryptoExpect<std::vector<uint8_t>>
ChaChaPolyCtx::encryptDetached(Span<uint8_t> Out, Span<const uint8_t> Data) {
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
  std::vector<uint8_t> RawTagData;
  RawTagData.reserve(TagLen);
  RawTagData.resize(TagLen);

  opensslAssuming(EVP_CIPHER_CTX_ctrl(Ctx.get(), EVP_CTRL_AEAD_GET_TAG, TagLen,
                                      RawTagData.data()));

  return RawTagData;
}

WasiCryptoExpect<__wasi_size_t>
ChaChaPolyCtx::decryptDetached(Span<uint8_t> Out, Span<const uint8_t> Data,
                               Span<const uint8_t> RawTag) {
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

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
