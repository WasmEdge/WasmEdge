// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/mac/hmac_sha2.h"
#include <openssl/rand.h>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

template <int Sha>
WasiCryptoExpect<std::unique_ptr<Key>>
HmacSha2<Sha>::KeyBuilder::generate(std::shared_ptr<Option>) {
  auto Len = keyLen();
  if (!Len) {
    return WasiCryptoUnexpect(Len);
  }
  std::vector<uint8_t> Raw(*Len, 0);
  RAND_bytes(Raw.data(), Raw.size());

  return std::make_unique<Key>(Alg, std::move(Raw));
}

template <int Sha>
WasiCryptoExpect<std::unique_ptr<Key>>
HmacSha2<Sha>::KeyBuilder::import(Span<uint8_t const> Raw) {
  return std::make_unique<Key>(Alg,
                               std::vector<uint8_t>{Raw.begin(), Raw.end()});
}

template <int Sha>
WasiCryptoExpect<__wasi_size_t> HmacSha2<Sha>::KeyBuilder::keyLen() {
  switch (Alg) {
  case SymmetricAlgorithm::HmacSha256:
    return 32;
  case SymmetricAlgorithm::HmacSha512:
    return 64;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
  }
}

template <int Sha>
WasiCryptoExpect<std::unique_ptr<typename HmacSha2<Sha>::State>>
HmacSha2<Sha>::State::open(std::shared_ptr<Key> OptKey,
                           std::shared_ptr<Option> OptOption) {
  ensureOrReturn(OptKey, __WASI_CRYPTO_ERRNO_KEY_REQUIRED);

  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> PKey{
      OptKey->inner().locked([](auto &Inner) {
        return EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, nullptr, Inner.Data.data(),
                                    Inner.Data.size());
      })};
  opensslAssuming(PKey);

  EVP_MD_CTX *Ctx = EVP_MD_CTX_new();
  opensslAssuming(Ctx);

  ensureOrReturn(
      EVP_DigestSignInit(Ctx, nullptr, ShaMap.at(Sha), nullptr, PKey.get()),
      __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return std::make_unique<HmacSha2<Sha>::State>(OptOption, Ctx);
}

template <int Sha>
WasiCryptoExpect<std::vector<uint8_t>>
HmacSha2<Sha>::State::optionsGet(std::string_view Name) {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->get(Name);
}

template <int Sha>
WasiCryptoExpect<uint64_t>
HmacSha2<Sha>::State::optionsGetU64(std::string_view Name) {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->getU64(Name);
}

template <int Sha>
WasiCryptoExpect<void> HmacSha2<Sha>::State::absorb(Span<const uint8_t> Data) {
  opensslAssuming(EVP_DigestSignUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

template <int Sha> WasiCryptoExpect<Tag> HmacSha2<Sha>::State::squeezeTag() {
  EVP_MD_CTX *CopyCtx = EVP_MD_CTX_new();
  opensslAssuming(EVP_MD_CTX_copy_ex(CopyCtx, Ctx.get()));

  size_t ActualOutSize;
  opensslAssuming(EVP_DigestSignFinal(CopyCtx, nullptr, &ActualOutSize));

  std::vector<uint8_t> Res(ActualOutSize);
  opensslAssuming(EVP_DigestSignFinal(CopyCtx, Res.data(), &ActualOutSize));

  return Res;
}

template class HmacSha2<256>;
template class HmacSha2<512>;

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
