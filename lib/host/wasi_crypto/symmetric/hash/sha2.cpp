// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/hash/sha2.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/evpwrapper.h"
#include "wasi_crypto/api.hpp"
#include <algorithm>
#include <array>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

template <uint32_t ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Sha2State<ShaNid>::optionsGet(std::string_view Name) {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->get(Name);
}

template <uint32_t ShaNid>
WasiCryptoExpect<uint64_t>
Sha2State<ShaNid>::optionsGetU64(std::string_view Name) {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->getU64(Name);
}

template <uint32_t ShaNid>
WasiCryptoExpect<void> Sha2State<ShaNid>::absorb(Span<uint8_t const> Data) {
  opensslAssuming(EVP_DigestUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

template <uint32_t ShaNid>
WasiCryptoExpect<void> Sha2State<ShaNid>::squeeze(Span<uint8_t> Out) {
  ensureOrReturn(getDigestSize() >= Out.size(),
                 __WASI_CRYPTO_ERRNO_INVALID_LENGTH);

  EvpMdCtxPtr CopyCtx{EVP_MD_CTX_new()};
  opensslAssuming(EVP_MD_CTX_copy_ex(CopyCtx.get(), Ctx.get()));

  if (getDigestSize() == Out.size()) {
    unsigned int Size;
    opensslAssuming(EVP_DigestFinal_ex(CopyCtx.get(), Out.data(), &Size));
    ensureOrReturn(Size == getDigestSize(),
                   __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  }

  if (getDigestSize() > Out.size()) {
    unsigned int Size;
    std::array<uint8_t, getDigestSize()> Cache;
    opensslAssuming(EVP_DigestFinal_ex(CopyCtx.get(), Cache.data(), &Size));
    ensureOrReturn(Size == getDigestSize(),
                   __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
    std::copy(Cache.begin(), Cache.begin() + Out.size(), Out.data());
  }

  return {};
}

template <uint32_t ShaNid>
WasiCryptoExpect<std::unique_ptr<Sha2State<ShaNid>>>
Sha2State<ShaNid>::open(std::shared_ptr<Options> OptOption) {
  EvpMdCtxPtr Ctx{EVP_MD_CTX_new()};
  opensslAssuming(EVP_DigestInit(Ctx.get(), EVP_get_digestbynid(ShaNid)));
  return std::make_unique<Sha2State>(OptOption, std::move(Ctx));
}

template class Sha2State<NID_sha256>;
template class Sha2State<NID_sha512>;
template class Sha2State<NID_sha512_256>;
} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
