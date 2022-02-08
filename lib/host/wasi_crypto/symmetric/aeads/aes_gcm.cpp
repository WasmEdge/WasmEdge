// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/aeads/aes_gcm.h"

#include "host/wasi_crypto/evpwrapper.h"
#include "openssl/rand.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {
namespace {} // namespace

template <uint32_t KeyBit>
WasiCryptoExpect<std::unique_ptr<Key>>
AesGcm<KeyBit>::KeyBuilder::generate(std::shared_ptr<Options>) {
  std::vector<uint8_t> Res(KeyBit / 8, 0);

  ensureOrReturn(Res.size() <= INT_MAX, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  ensureOrReturn(RAND_bytes(Res.data(), static_cast<int>(Res.size())),
                 __WASI_CRYPTO_ERRNO_RNG_ERROR);

  return std::make_unique<Key>(Alg, std::move(Res));
}

template <uint32_t KeyBit>
WasiCryptoExpect<std::unique_ptr<Key>>
AesGcm<KeyBit>::KeyBuilder::import(Span<uint8_t const> Raw) {
  return std::make_unique<Key>(Alg,
                               std::vector<uint8_t>{Raw.begin(), Raw.end()});
}

template <uint32_t KeyBit>
WasiCryptoExpect<std::unique_ptr<typename AesGcm<KeyBit>::State>>
AesGcm<KeyBit>::State::open(std::shared_ptr<Key> OptKey,
                            std::shared_ptr<Options> OptOption) {
  ensureOrReturn(OptKey, __WASI_CRYPTO_ERRNO_KEY_REQUIRED);
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_NONCE_REQUIRED);

  auto Nonce = OptOption->get("nonce");
  if (!Nonce) {
    return WasiCryptoUnexpect(Nonce);
  }

  ensureOrReturn(Nonce->size() == NonceSize,
                 __WASI_CRYPTO_ERRNO_INVALID_HANDLE);

  ensureOrReturn(KeyBit / 8 == OptKey->data().size(),
                 __WASI_CRYPTO_ERRNO_INVALID_HANDLE);

  EvpCipherCtxPtr Ctx{EVP_CIPHER_CTX_new()};
  opensslAssuming(EVP_CipherInit(Ctx.get(), getCipher(), OptKey->data().data(),
                                 Nonce->data(), Mode::Unchanged));

  return std::make_unique<State>(std::move(Ctx), OptOption);
}

template <uint32_t KeyBit>
WasiCryptoExpect<std::vector<uint8_t>>
AesGcm<KeyBit>::State::optionsGet(std::string_view Name) {
  return OptOption->get(Name);
}

template <uint32_t KeyBit>
WasiCryptoExpect<uint64_t>
AesGcm<KeyBit>::State::optionsGetU64(std::string_view Name) {
  return OptOption->getU64(Name);
}

template <uint32_t KeyBit>
WasiCryptoExpect<void> AesGcm<KeyBit>::State::absorb(Span<const uint8_t> Data) {
  ensureOrReturn(Data.size() <= INT_MAX, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  int Len;
  opensslAssuming(EVP_CipherUpdate(Ctx.get(), nullptr, &Len, Data.data(),
                                   static_cast<int>(Data.size())));

  return {};
}

template <uint32_t KeyBit>
WasiCryptoExpect<Tag>
AesGcm<KeyBit>::State::encryptDetachedUnchecked(Span<uint8_t> Out,
                                                Span<const uint8_t> Data) {

  opensslAssuming(EVP_CipherInit_ex(Ctx.get(), nullptr, nullptr, nullptr,
                                    nullptr, Mode::Encrypt));

  ensureOrReturn(Data.size() <= INT_MAX, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  int ActualOutSize;
  opensslAssuming(EVP_CipherUpdate(Ctx.get(), Out.data(), &ActualOutSize,
                                   Data.data(), static_cast<int>(Data.size())));

  // we need check the equal.
  if (ActualOutSize < 0 || static_cast<size_t>(ActualOutSize) != Out.size()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_NONCE);
  }

  int AL;
  opensslAssuming(EVP_CipherFinal_ex(Ctx.get(), nullptr, &AL));

  // Gen tag
  std::vector<uint8_t> RawTagData(TagLen);

  opensslAssuming(EVP_CIPHER_CTX_ctrl(Ctx.get(), EVP_CTRL_AEAD_GET_TAG,
                                      static_cast<int>(TagLen),
                                      RawTagData.data()));

  return RawTagData;
}

template <uint32_t KeyBit>
WasiCryptoExpect<size_t> AesGcm<KeyBit>::State::decryptDetachedUnchecked(
    Span<uint8_t> Out, Span<const uint8_t> Data, Span<uint8_t const> RawTag) {
  opensslAssuming(EVP_CipherInit_ex(Ctx.get(), nullptr, nullptr, nullptr,
                                    nullptr, Mode::Decrypt));

  ensureOrReturn(Data.size() <= INT_MAX, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  int ActualOutSize;
  opensslAssuming(EVP_CipherUpdate(Ctx.get(), Out.data(), &ActualOutSize,
                                   Data.data(), static_cast<int>(Data.size())));
  ensureOrReturn(ActualOutSize == static_cast<int>(Data.size()),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  opensslAssuming(EVP_CIPHER_CTX_ctrl(Ctx.get(), EVP_CTRL_AEAD_SET_TAG,
                                      static_cast<int>(TagLen),
                                      const_cast<uint8_t *>(RawTag.data())));

  // Construct a temp var
  int OutLen;
  ensureOrReturn(EVP_CipherFinal_ex(Ctx.get(), nullptr, &OutLen),
                 __WASI_CRYPTO_ERRNO_INVALID_TAG);
  ensureOrReturn(OutLen == 0, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  ensureOrReturn(ActualOutSize >= 0, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  return static_cast<size_t>(ActualOutSize);
}

template class AesGcm<128>;
template class AesGcm<256>;

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
