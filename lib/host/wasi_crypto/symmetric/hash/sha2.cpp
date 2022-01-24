// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/hash/sha2.h"
#include "host/wasi_crypto/evpwrapper.h"

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

  EvpMdCtxPtr CopyCtx{EVP_MD_CTX_new()};
  opensslAssuming(EVP_MD_CTX_copy_ex(CopyCtx.get(), Ctx.get()));

  std::vector<uint8_t> Buffer(
      static_cast<size_t>(EVP_MD_CTX_size(CopyCtx.get())));
  ensureOrReturn(Buffer.size() >= Out.size(),
                 __WASI_CRYPTO_ERRNO_INVALID_LENGTH);

  unsigned int Size = 0;
  opensslAssuming(EVP_DigestFinal_ex(CopyCtx.get(), Buffer.data(), &Size));
  opensslAssuming(Size == Buffer.size());

  std::copy(Buffer.begin(), Buffer.begin() + Out.size(), Out.data());
  return {};
}

template <uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<Sha2State<Sha>>>
Sha2State<Sha>::open(std::shared_ptr<Key> OptKey,
                     std::shared_ptr<Options> OptOption) {
  ensureOrReturn(!OptKey, __WASI_CRYPTO_ERRNO_KEY_NOT_SUPPORTED);

  EvpMdCtxPtr Ctx{EVP_MD_CTX_new()};

  opensslAssuming(EVP_DigestInit(Ctx.get(), ShaMap.at(Sha)));

  return std::make_unique<Sha2State>(OptOption, std::move(Ctx));
}

template class Sha2State<256>;
template class Sha2State<512>;
template class Sha2State<512256>;
} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
