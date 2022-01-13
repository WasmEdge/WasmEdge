// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/hash/sha2.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

template <uint32_t Sha>
WasiCryptoExpect<std::vector<uint8_t>>
Sha2State<Sha>::optionsGet(std::string_view Name) {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->get(Name);
}

template <uint32_t Sha>
WasiCryptoExpect<uint64_t>
Sha2State<Sha>::optionsGetU64(std::string_view Name) {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->getU64(Name);
}

template <uint32_t Sha>
WasiCryptoExpect<void> Sha2State<Sha>::absorb(Span<uint8_t const> Data) {
  opensslAssuming(EVP_DigestUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

template <uint32_t Sha>
WasiCryptoExpect<void> Sha2State<Sha>::squeeze(Span<uint8_t> Out) {

  // If finalization is required, the implementation MUST duplicate the internal
  // state and apply the finalization on the copy, leaving the state unchanged
  // from the guest perspective.

  EVP_MD_CTX *CopyCtx = EVP_MD_CTX_new();
  opensslAssuming(EVP_MD_CTX_copy_ex(CopyCtx, Ctx.get()));

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

  std::copy(Cache.begin(), Cache.end(), Out.data());

  return {};
}

template <uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<Sha2State<Sha>>>
Sha2State<Sha>::open(std::shared_ptr<Key> OptKey,
                     std::shared_ptr<Options> OptOption) {
  ensureOrReturn(!OptKey, __WASI_CRYPTO_ERRNO_KEY_NOT_SUPPORTED);

  EVP_MD_CTX *Ctx = EVP_MD_CTX_new();
  opensslAssuming(Ctx);

  EVP_MD const *Md = ShaMap.at(Sha);

  opensslAssuming(EVP_DigestInit(Ctx, Md));

  return std::make_unique<Sha2State>(OptOption, Ctx);
}

template class Sha2State<256>;
template class Sha2State<512>;
template class Sha2State<512256>;
} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
