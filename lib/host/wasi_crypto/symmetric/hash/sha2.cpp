// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/hash/sha2.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {
namespace {
constexpr EVP_MD const *getMd(SymmetricAlgorithm Alg) {
  switch (Alg) {
  case SymmetricAlgorithm::Sha256:
    return EVP_sha256();
    break;
  case SymmetricAlgorithm::Sha512:
    return EVP_sha512();
    break;
  case SymmetricAlgorithm::Sha512_256:
    return EVP_sha512_256();
  default:
    __builtin_unreachable();
  }
}
} // namespace

WasiCryptoExpect<std::vector<uint8_t>>
Sha2State::optionsGet(std::string_view Name) {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->get(Name);
}

WasiCryptoExpect<uint64_t> Sha2State::optionsGetU64(std::string_view Name) {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->getU64(Name);
}

WasiCryptoExpect<void> Sha2State::absorb(Span<uint8_t const> Data) {
  opensslAssuming(EVP_DigestUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

WasiCryptoExpect<void> Sha2State::squeeze(Span<uint8_t> Out) {

  // If finalization is required, the implementation MUST duplicate the internal
  // state and apply the finalization on the copy, leaving the state unchanged
  // from the guest perspective.

  // allocate by EVP_MD_CTX_copy
  EVP_MD_CTX *CopyCtx = nullptr;
  opensslAssuming(EVP_MD_CTX_copy(CopyCtx, Ctx.get()));

  // Note: just copy `Out.size()` length from ctx. However, OpenSSL don't have
  // such a function, it will copy `EVP_MD_CTX_size(CopyCtx)`, so create Cache
  std::vector<uint8_t> Cache(EVP_MD_CTX_size(CopyCtx));
  ensureOrReturn(Cache.size() >= Out.size(),
                 __WASI_CRYPTO_ERRNO_INVALID_LENGTH);

  unsigned int Size = 0;
  // Auto clean CopyCtx, not leak
  opensslAssuming(EVP_DigestFinal(CopyCtx, Cache.data(), &Size));

  // Check
  opensslAssuming(Size == Cache.size());

  std::copy(Cache.begin(), Cache.begin() + Size, Out.data());

  return {};
}

WasiCryptoExpect<std::unique_ptr<Sha2State>>
Sha2State::open(SymmetricAlgorithm Alg, std::shared_ptr<Key> OptKey,
                std::shared_ptr<Option> OptOption) {
  ensureOrReturn(!OptKey, __WASI_CRYPTO_ERRNO_KEY_NOT_SUPPORTED);

  EVP_MD_CTX *Ctx = EVP_MD_CTX_new();
  opensslAssuming(Ctx);

  EVP_MD const *Md = getMd(Alg);

  opensslAssuming(EVP_DigestInit(Ctx, Md));

  return std::make_unique<Sha2State>(OptOption, Ctx);
}
} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
