// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/mac/hmac_sha2.h"
#include "host/wasi_crypto/error.h"
#include "wasi_crypto/api.hpp"
#include <openssl/rand.h>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

template <uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<Key>>
HmacSha2<Sha>::KeyBuilder::generate(std::shared_ptr<Options>) {
  std::vector<uint8_t> Raw(Sha / 8, 0);
  ensureOrReturn(RAND_bytes(Raw.data(), static_cast<int>(Raw.size())),
                 __WASI_CRYPTO_ERRNO_RNG_ERROR);

  return std::make_unique<Key>(Alg, std::move(Raw));
}

template <uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<Key>>
HmacSha2<Sha>::KeyBuilder::import(Span<uint8_t const> Raw) {
  return std::make_unique<Key>(Alg,
                               std::vector<uint8_t>{Raw.begin(), Raw.end()});
}

template <uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<typename HmacSha2<Sha>::State>>
HmacSha2<Sha>::State::open(std::shared_ptr<Key> OptKey,
                           std::shared_ptr<Options> OptOption) {
  ensureOrReturn(OptKey, __WASI_CRYPTO_ERRNO_KEY_REQUIRED);

  EvpPkeyPtr PKey{EVP_PKEY_new_raw_private_key(
      EVP_PKEY_HMAC, nullptr, OptKey->data().data(), OptKey->data().size())};
  EvpMdCtxPtr Ctx{EVP_MD_CTX_new()};

  opensslAssuming(EVP_DigestSignInit(Ctx.get(), nullptr, ShaMap.at(Sha),
                                     nullptr, PKey.get()));

  return std::make_unique<HmacSha2<Sha>::State>(OptOption, std::move(Ctx));
}

template <uint32_t Sha>
WasiCryptoExpect<std::vector<uint8_t>>
HmacSha2<Sha>::State::optionsGet(std::string_view Name) {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->get(Name);
}

template <uint32_t Sha>
WasiCryptoExpect<uint64_t>
HmacSha2<Sha>::State::optionsGetU64(std::string_view Name) {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->getU64(Name);
}

template <uint32_t Sha>
WasiCryptoExpect<void> HmacSha2<Sha>::State::absorb(Span<const uint8_t> Data) {
  opensslAssuming(EVP_DigestSignUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

template <uint32_t Sha>
WasiCryptoExpect<Tag> HmacSha2<Sha>::State::squeezeTag() {
  size_t ActualOutSize;

  std::vector<uint8_t> Res(Sha / 8);
  opensslAssuming(EVP_DigestSignFinal(Ctx.get(), Res.data(), &ActualOutSize));
  ensureOrReturn(ActualOutSize == Sha / 8,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  return Res;
}

template class HmacSha2<256>;
template class HmacSha2<512>;

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
