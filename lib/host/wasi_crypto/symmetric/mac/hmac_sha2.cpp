// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/mac/hmac_sha2.h"
#include <openssl/rand.h>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {
namespace {
constexpr EVP_MD const *getMd(SymmetricAlgorithm Alg) {
  switch (Alg) {
  case SymmetricAlgorithm::HmacSha256:
    return EVP_sha256();
    break;
  case SymmetricAlgorithm::HmacSha512:
    return EVP_sha512();
    break;
  default:
    __builtin_unreachable();
  }
}
} // namespace

WasiCryptoExpect<std::unique_ptr<Key>>
HmacSha2KeyBuilder::generate(std::shared_ptr<Option>) {
  auto Len = keyLen();
  if (!Len) {
    return WasiCryptoUnexpect(Len);
  }
  std::vector<uint8_t> Raw(*Len, 0);
  RAND_bytes(Raw.data(), Raw.size());

  return std::make_unique<Key>(Alg, std::move(Raw));
}

WasiCryptoExpect<std::unique_ptr<Key>>
HmacSha2KeyBuilder::import(Span<uint8_t const> Raw) {
  return std::make_unique<Key>(Alg,
                               std::vector<uint8_t>{Raw.begin(), Raw.end()});
}

WasiCryptoExpect<__wasi_size_t> HmacSha2KeyBuilder::keyLen() {
  switch (Alg) {
  case SymmetricAlgorithm::HmacSha256:
    return 32;
  case SymmetricAlgorithm::HmacSha512:
    return 64;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
  }
}

WasiCryptoExpect<std::unique_ptr<HmacSha2State>>
HmacSha2State::open(SymmetricAlgorithm Alg, std::shared_ptr<Key> OptKey,
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
      EVP_DigestSignInit(Ctx, nullptr, getMd(Alg), nullptr, PKey.get()),
      __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return std::make_unique<HmacSha2State>(OptOption, Ctx);
}

WasiCryptoExpect<std::vector<uint8_t>>
HmacSha2State::optionsGet(std::string_view Name) {

  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->get(Name);
}

WasiCryptoExpect<uint64_t> HmacSha2State::optionsGetU64(std::string_view Name) {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->getU64(Name);
}

WasiCryptoExpect<void> HmacSha2State::absorb(Span<const uint8_t> Data) {
  opensslAssuming(EVP_DigestSignUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

WasiCryptoExpect<Tag> HmacSha2State::squeezeTag() {
  EVP_MD_CTX *CopyCtx = nullptr;
  opensslAssuming(EVP_MD_CTX_copy(CopyCtx, Ctx.get()));

  size_t ActualOutSize;
  opensslAssuming(EVP_DigestSignFinal(CopyCtx, nullptr, &ActualOutSize));

  std::vector<uint8_t> Res(ActualOutSize);
  opensslAssuming(EVP_DigestSignFinal(CopyCtx, Res.data(), &ActualOutSize));

  return Res;
}

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
