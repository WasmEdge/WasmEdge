// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/aeads/chacha_poly.h"

#include <map>
#include <openssl/rand.h>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

template <uint32_t NonceBit>
WasiCryptoExpect<std::unique_ptr<typename ChaChaPoly<NonceBit>::State>>
ChaChaPoly<NonceBit>::State::open(std::shared_ptr<Key> OptKey,
                                  std::shared_ptr<Options> OptOption) {
  ensureOrReturn(OptKey, __WASI_CRYPTO_ERRNO_KEY_REQUIRED);
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_NONCE_REQUIRED);

  auto Nonce = OptOption->get("nonce");
  if (!Nonce) {
    return WasiCryptoUnexpect(Nonce);
  }

  ensureOrReturn(Nonce->size() == NonceBit / 8,
                 __WASI_CRYPTO_ERRNO_INVALID_HANDLE);

  EVP_CIPHER_CTX *Ctx = EVP_CIPHER_CTX_new();
  EVP_CIPHER_CTX_ctrl(Ctx, EVP_CTRL_AEAD_SET_IVLEN, NonceBit / 8, nullptr);

  opensslAssuming(Ctx);
  opensslAssuming(EVP_CipherInit_ex(Ctx, EVP_chacha20_poly1305(), nullptr,
                                    OptKey->data().data(), Nonce->data(),
                                    Mode::Unchanged));

  return std::make_unique<ChaChaPoly<NonceBit>::State>(Ctx, OptOption);
}

template <uint32_t NonceBit>
WasiCryptoExpect<std::unique_ptr<Key>>
ChaChaPoly<NonceBit>::KeyBuilder::generate(std::shared_ptr<Options>) {
  std::vector<uint8_t> Res(keyLen(), 0);

  ensureOrReturn(Res.size() <= INT_MAX, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  ensureOrReturn(RAND_bytes(Res.data(), static_cast<int>(Res.size())),
                 __WASI_CRYPTO_ERRNO_RNG_ERROR);

  return std::make_unique<Key>(Alg, std::move(Res));
}

template <uint32_t NonceBit>
WasiCryptoExpect<std::unique_ptr<Key>>
ChaChaPoly<NonceBit>::KeyBuilder::import(Span<uint8_t const> Raw) {
  return std::make_unique<Key>(Alg,
                               std::vector<uint8_t>{Raw.begin(), Raw.end()});
}

template <uint32_t NonceBit>
__wasi_size_t ChaChaPoly<NonceBit>::KeyBuilder::keyLen() {
  return 32;
}

template <uint32_t NonceBit>
WasiCryptoExpect<std::vector<uint8_t>>
ChaChaPoly<NonceBit>::State::optionsGet(std::string_view Name) {
  return OptOptions->get(Name);
}

template <uint32_t NonceBit>
WasiCryptoExpect<uint64_t>
ChaChaPoly<NonceBit>::State::optionsGetU64(std::string_view Name) {
  return OptOptions->getU64(Name);
}

template <uint32_t NonceBit>
WasiCryptoExpect<void>
ChaChaPoly<NonceBit>::State::absorb(Span<const uint8_t> Data) {
  {
    ensureOrReturn(Data.size() <= INT_MAX,
                   __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
    int Temp;
    opensslAssuming(EVP_CipherUpdate(Ctx.get(), nullptr, &Temp, Data.data(),
                                     static_cast<int>(Data.size())));
  }

  return {};
}

template <uint32_t NonceBit>
WasiCryptoExpect<Tag> ChaChaPoly<NonceBit>::State::encryptDetachedUnchecked(
    Span<uint8_t> Out, Span<const uint8_t> Data) {
  opensslAssuming(EVP_CipherInit_ex(Ctx.get(), nullptr, nullptr, nullptr,
                                    nullptr, Mode::Encrypt));

  int ActualOutSize;
  ensureOrReturn(Data.size() <= INT_MAX, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  opensslAssuming(EVP_CipherUpdate(Ctx.get(), Out.data(), &ActualOutSize,
                                   Data.data(), static_cast<int>(Data.size())));

  // we need check the equal.
  if (ActualOutSize < 0 ||
      static_cast<__wasi_size_t>(ActualOutSize) != Out.size()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_NONCE);
  }

  {
    int Temp;
    opensslAssuming(EVP_CipherFinal_ex(Ctx.get(), nullptr, &Temp));
  }

  // Gen tag
  std::vector<uint8_t> RawTagData(TagLen);

  opensslAssuming(EVP_CIPHER_CTX_ctrl(Ctx.get(), EVP_CTRL_AEAD_GET_TAG,
                                      static_cast<int>(TagLen),
                                      RawTagData.data()));

  return RawTagData;
}

template <uint32_t NonceBit>
WasiCryptoExpect<__wasi_size_t>
ChaChaPoly<NonceBit>::State::decryptDetachedUnchecked(
    Span<uint8_t> Out, Span<const uint8_t> Data, Span<uint8_t const> RawTag) {
  opensslAssuming(EVP_CipherInit_ex(Ctx.get(), nullptr, nullptr, nullptr,
                                    nullptr, Mode::Decrypt));

  int ActualOutSize;
  ensureOrReturn(Data.size() <= INT_MAX, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  opensslAssuming(EVP_CipherUpdate(Ctx.get(), Out.data(), &ActualOutSize,
                                   Data.data(), static_cast<int>(Data.size())));

  opensslAssuming(EVP_CIPHER_CTX_ctrl(Ctx.get(), EVP_CTRL_AEAD_SET_TAG,
                                      static_cast<int>(TagLen),
                                      const_cast<uint8_t *>(RawTag.data())));

  {
    int Temp = 0;
    ensureOrReturn(EVP_CipherFinal_ex(Ctx.get(), nullptr, &Temp),
                   __WASI_CRYPTO_ERRNO_INVALID_TAG);
  }

  ensureOrReturn(ActualOutSize >= 0, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  return static_cast<__wasi_size_t>(ActualOutSize);
}

template <uint32_t NonceBit>
WasiCryptoExpect<__wasi_size_t> ChaChaPoly<NonceBit>::State::maxTagLen() {
  return TagLen;
}

template class ChaChaPoly<96>;
// template class ChaChaPoly<192>;
} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
