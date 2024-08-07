// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "symmetric/hash.h"
#include "utils/evp_wrapper.h"

#include <openssl/evp.h>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

template <int ShaNid> constexpr size_t Sha2<ShaNid>::getDigestSize() noexcept {
  static_assert(ShaNid == NID_sha256 || ShaNid == NID_sha512 ||
                ShaNid == NID_sha512_256);
  if constexpr (ShaNid == NID_sha256)
    return 32;
  if constexpr (ShaNid == NID_sha512)
    return 64;
  if constexpr (ShaNid == NID_sha512_256)
    return 32;
}

template <int ShaNid>
WasiCryptoExpect<typename Sha2<ShaNid>::State>
Sha2<ShaNid>::State::open(OptionalRef<const Options>) noexcept {
  EvpMdCtxPtr Ctx{EVP_MD_CTX_new()};
  opensslCheck(EVP_DigestInit(Ctx.get(), EVP_get_digestbynid(ShaNid)));
  return Ctx;
}

template <int ShaNid>
WasiCryptoExpect<void>
Sha2<ShaNid>::State::absorb(Span<const uint8_t> Data) noexcept {
  std::unique_lock Lock{Ctx->Mutex};
  opensslCheck(EVP_DigestUpdate(Ctx->RawCtx.get(), Data.data(), Data.size()));
  return {};
}

template <int ShaNid>
WasiCryptoExpect<void>
Sha2<ShaNid>::State::squeeze(Span<uint8_t> Out) noexcept {
  ensureOrReturn(getDigestSize() >= Out.size(),
                 __WASI_CRYPTO_ERRNO_INVALID_LENGTH);

  EvpMdCtxPtr CopyCtx{EVP_MD_CTX_new()};

  {
    std::shared_lock Lock{Ctx->Mutex};
    opensslCheck(EVP_MD_CTX_copy_ex(CopyCtx.get(), Ctx->RawCtx.get()));
  }

  if (getDigestSize() == Out.size()) {
    unsigned int Size;
    opensslCheck(EVP_DigestFinal_ex(CopyCtx.get(), Out.data(), &Size));
    ensureOrReturn(Size == getDigestSize(),
                   __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  }

  if (getDigestSize() > Out.size()) {
    unsigned int Size;
    std::array<uint8_t, getDigestSize()> Cache;
    opensslCheck(EVP_DigestFinal_ex(CopyCtx.get(), Cache.data(), &Size));
    ensureOrReturn(Size == getDigestSize(),
                   __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
    std::copy(Cache.begin(), Cache.begin() + static_cast<ptrdiff_t>(Out.size()),
              Out.data());
  }

  return {};
}

template <int ShaNid>
WasiCryptoExpect<typename Sha2<ShaNid>::State>
Sha2<ShaNid>::State::clone() const noexcept {
  EvpMdCtxPtr CloneCtx{EVP_MD_CTX_new()};

  {
    std::shared_lock Lock{Ctx->Mutex};
    opensslCheck(EVP_MD_CTX_copy_ex(CloneCtx.get(), Ctx->RawCtx.get()));
  }

  return CloneCtx;
}

template class Sha2<NID_sha256>;
template class Sha2<NID_sha512>;
template class Sha2<NID_sha512_256>;

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
