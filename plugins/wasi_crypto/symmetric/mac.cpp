// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "symmetric/mac.h"
#include "utils/secret_vec.h"

#include <openssl/rand.h>

#include <mutex>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

template <int ShaNid> constexpr size_t Hmac<ShaNid>::getKeySize() noexcept {
  static_assert(ShaNid == NID_sha256 || ShaNid == NID_sha512);
  if constexpr (ShaNid == NID_sha256) {
    return 32;
  }
  if constexpr (ShaNid == NID_sha512) {
    return 64;
  }
}

template <int ShaNid>
WasiCryptoExpect<typename Hmac<ShaNid>::Key>
Hmac<ShaNid>::Key::generate(OptionalRef<const Options>) noexcept {
  return SecretVec::random<getKeySize()>();
}

template <int ShaNid>
WasiCryptoExpect<typename Hmac<ShaNid>::Key>
Hmac<ShaNid>::Key::import(Span<const uint8_t> Raw) noexcept {
  return SecretVec{Raw};
}

template <int ShaNid>
WasiCryptoExpect<typename Hmac<ShaNid>::State>
Hmac<ShaNid>::State::open(const Key &Key, OptionalRef<const Options>) noexcept {
  EvpPkeyPtr HmacKey{EVP_PKEY_new_raw_private_key(
      EVP_PKEY_HMAC, nullptr, Key.ref().data(), Key.ref().size())};
  opensslCheck(HmacKey);

  EvpMdCtxPtr Ctx{EVP_MD_CTX_new()};

  opensslCheck(EVP_DigestSignInit(
      Ctx.get(), nullptr, EVP_get_digestbynid(ShaNid), nullptr, HmacKey.get()));

  return Ctx;
}

template <int ShaNid>
WasiCryptoExpect<void>
Hmac<ShaNid>::State::absorb(Span<const uint8_t> Data) noexcept {
  std::scoped_lock Lock{Ctx->Mutex};

  opensslCheck(
      EVP_DigestSignUpdate(Ctx->RawCtx.get(), Data.data(), Data.size()));
  return {};
}

template <int ShaNid>
WasiCryptoExpect<Tag> Hmac<ShaNid>::State::squeezeTag() noexcept {
  size_t ActualOutSize = getKeySize();
  SecretVec Res(ActualOutSize);
  {
    std::scoped_lock Lock{Ctx->Mutex};
    opensslCheck(
        EVP_DigestSignFinal(Ctx->RawCtx.get(), Res.data(), &ActualOutSize));
  }

  ensureOrReturn(ActualOutSize == getKeySize(),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  return Res;
}

template <int ShaNid>
WasiCryptoExpect<typename Hmac<ShaNid>::State>
Hmac<ShaNid>::State::clone() const noexcept {
  EvpMdCtxPtr CloneCtx{EVP_MD_CTX_new()};

  {
    std::scoped_lock Lock{Ctx->Mutex};
    opensslCheck(EVP_MD_CTX_copy_ex(CloneCtx.get(), Ctx->RawCtx.get()));
  }

  return CloneCtx;
}

template class Hmac<NID_sha256>;
template class Hmac<NID_sha512>;

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
