// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/aeads/charcha_poly.h"
#include "host/wasi_crypto/wrapper/random.h"

#include <map>
namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {


namespace {
const std::map<SymmetricAlgorithm, size_t> NonceMap{
    {SymmetricAlgorithm::ChaCha20Poly1305, 12},
    {SymmetricAlgorithm::XChaCha20Poly1305, 24}};
}
ChaChaPolyKeyBuilder::ChaChaPolyKeyBuilder(SymmetricAlgorithm Alg) : Alg{Alg} {}

WasiCryptoExpect<Key> ChaChaPolyKeyBuilder::generate(std::shared_ptr<Options>) {
  auto Len = keyLen();
  CryptoRandom Random;
  if (!Len) {
    return WasiCryptoUnexpect(Len);
  }
  std::vector<uint8_t> Raw(*Len, 0);
  if (auto Res = Random.fill(Raw); !Res.has_value()) {
    return WasiCryptoUnexpect(Res);
  }

  return import(Raw);
}

WasiCryptoExpect<Key> ChaChaPolyKeyBuilder::import(Span<uint8_t const> Raw) {
  return Key{std::make_unique<ChaChaPolyKey>(Alg, Raw)};
}

WasiCryptoExpect<__wasi_size_t> ChaChaPolyKeyBuilder::keyLen() {
  switch (Alg) {
  case SymmetricAlgorithm::ChaCha20Poly1305:
    return 16;
  case SymmetricAlgorithm::XChaCha20Poly1305:
    return 32;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
  }
}

WasiCryptoExpect<std::unique_ptr<ChaChaPolySymmetricState>>
ChaChaPolySymmetricState::make(SymmetricAlgorithm, std::optional<Key>,
                               std::shared_ptr<Options>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::vector<uint8_t>>
ChaChaPolySymmetricState::optionsGet(std::string_view Name) {
  return Options.get(Name);
}

WasiCryptoExpect<uint64_t>
ChaChaPolySymmetricState::optionsGetU64(std::string_view Name) {
  return Options.getU64(Name);
}

WasiCryptoExpect<void>
ChaChaPolySymmetricState::absorb(Span<const uint8_t> Data) {
  int Len;
  // TODO: need change Openssl AAD default length from 12 if beyond?
  opensslAssuming(
      EVP_CipherUpdate(Ctx.get(), nullptr, &Len, Data.data(), Data.size()));

  return {};
}

WasiCryptoExpect<Tag>
ChaChaPolySymmetricState::encryptDetachedUnchecked(Span<uint8_t> Out,
                                                   Span<const uint8_t> Data) {
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
ChaChaPolySymmetricState::decryptDetachedUnchecked(Span<uint8_t> Out,
                                                   Span<const uint8_t> Data,
                                                   Span<uint8_t const> RawTag) {
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

  opensslAssuming(EVP_CipherInit_ex(Ctx.get(), EVP_chacha20_poly1305(), nullptr,
                                    Key.data(), Nonce.data(), Mode::Unchanged));

  return ChaChaPolyCtx{Alg, std::move(Ctx)};
}



} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
