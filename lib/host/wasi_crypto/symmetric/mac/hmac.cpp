// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/symmetric/mac/hmac.h"
#include "host/wasi_crypto/utils/secret_vec.h"
#include "openssl/rand.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

template <int ShaNid> constexpr size_t Hmac<ShaNid>::getKeySize() noexcept {
  if constexpr (ShaNid == NID_sha256) {
    return 32;
  }
  if constexpr (ShaNid == NID_sha512) {
    return 64;
  }
}

template <int ShaNid>
WasiCryptoExpect<typename Hmac<ShaNid>::Key>
Hmac<ShaNid>::Key::generate(OptionalRef<Options>) noexcept {
  return SecretVec::random<getKeySize()>();
}

template <int ShaNid>
WasiCryptoExpect<typename Hmac<ShaNid>::Key>
Hmac<ShaNid>::Key::import(Span<const uint8_t> Raw) noexcept {
  return std::make_shared<SecretVec>(Raw);
}

template <int ShaNid>
WasiCryptoExpect<typename Hmac<ShaNid>::State>
Hmac<ShaNid>::State::open(Key &Key, OptionalRef<Options>) noexcept {
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
  opensslCheck(EVP_DigestSignUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

template <int ShaNid>
WasiCryptoExpect<Tag> Hmac<ShaNid>::State::squeezeTag() noexcept {
  std::vector<uint8_t> Res(getKeySize());

  size_t ActualOutSize;
  opensslCheck(EVP_DigestSignFinal(Ctx.get(), Res.data(), &ActualOutSize));
  ensureOrReturn(ActualOutSize == getKeySize(),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  return Res;
}

template class Hmac<NID_sha256>;
template class Hmac<NID_sha512>;

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge