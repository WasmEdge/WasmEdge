// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/extract_and_expand/hkdf.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/evpwrapper.h"
#include "host/wasi_crypto/util.h"
#include "openssl/rand.h"
#include "wasi_crypto/api.hpp"
#include <openssl/evp.h>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

template <uint32_t ShaNid, uint32_t Mode>
WasiCryptoExpect<std::unique_ptr<Key>>
Hkdf<ShaNid, Mode>::KeyBuilder::generate(std::shared_ptr<Options>) {
  std::vector<uint8_t> Data(getKeySize());
  ensureOrReturn(RAND_bytes(Data.data(), static_cast<int>(Data.size())),
                 __WASI_CRYPTO_ERRNO_RNG_ERROR);

  return std::make_unique<Key>(Alg, std::move(Data));
}

template <uint32_t ShaNid, uint32_t Mode>
WasiCryptoExpect<std::unique_ptr<Key>>
Hkdf<ShaNid, Mode>::KeyBuilder::import(Span<uint8_t const> Raw) {
  return std::make_unique<Key>(Alg,
                               std::vector<uint8_t>{Raw.begin(), Raw.end()});
}

template <uint32_t ShaNid, uint32_t Mode>
WasiCryptoExpect<std::unique_ptr<typename Hkdf<ShaNid, Mode>::State>>
Hkdf<ShaNid, Mode>::State::open(std::shared_ptr<Key> OptKey,
                                std::shared_ptr<Options> OptOption) {
  ensureOrReturn(OptKey, __WASI_CRYPTO_ERRNO_KEY_REQUIRED);

  EvpPkeyCtxPtr Ctx{EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr)};
  opensslAssuming(Ctx);
  opensslAssuming(EVP_PKEY_derive_init(Ctx.get()));
  opensslAssuming(EVP_PKEY_CTX_set_hkdf_md(Ctx.get(), getShaCtx()));
  opensslAssuming(EVP_PKEY_CTX_hkdf_mode(Ctx.get(), Mode));
  opensslAssuming(EVP_PKEY_CTX_set1_hkdf_key(Ctx.get(), OptKey->data().data(),
                                             OptKey->data().size()));

  return std::make_unique<State>(OptOption, std::move(Ctx));
}

template <uint32_t ShaNid, uint32_t Mode>
WasiCryptoExpect<void>
Hkdf<ShaNid, Mode>::State::absorb(Span<const uint8_t> Data) {
  std::unique_lock<std::shared_mutex> Lock{Mutex};

  Cache.insert(Cache.end(), Data.begin(), Data.end());
  return {};
}

template <uint32_t ShaNid, uint32_t Mode>
WasiCryptoExpect<std::unique_ptr<Key>>
Hkdf<ShaNid, Mode>::State::squeezeKey(SymmetricAlgorithm InputAlg) {
  if constexpr (Mode != EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  std::shared_lock<std::shared_mutex> Lock{Mutex};

  opensslAssuming(
      EVP_PKEY_CTX_set1_hkdf_salt(Ctx.get(), Cache.data(), Cache.size()));

  std::vector<uint8_t> Data(getKeySize());
  size_t Size;
  opensslAssuming(EVP_PKEY_derive(Ctx.get(), Data.data(), &Size));
  ensureOrReturn(Size == getKeySize(), __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return std::make_unique<Key>(InputAlg, std::move(Data));
}

template <uint32_t ShaNid, uint32_t Mode>
WasiCryptoExpect<void> Hkdf<ShaNid, Mode>::State::squeeze(Span<uint8_t> Out) {
  if constexpr (Mode != EVP_PKEY_HKDEF_MODE_EXPAND_ONLY) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  opensslAssuming(
      EVP_PKEY_CTX_add1_hkdf_info(Ctx.get(), Cache.data(), Cache.size()));
  ensureOrReturn(
      EVP_PKEY_derive(Ctx.get(), Out.data(), addressOfTempory(Out.size())),
      __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return {};
}

template <uint32_t ShaNid, uint32_t Mode>
WasiCryptoExpect<std::vector<uint8_t>>
Hkdf<ShaNid, Mode>::State::optionsGet(std::string_view Name) {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->get(Name);
}

template <uint32_t ShaNid, uint32_t Mode>
WasiCryptoExpect<uint64_t>
Hkdf<ShaNid, Mode>::State::optionsGetU64(std::string_view Name) {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->getU64(Name);
}

template <uint32_t ShaNid, uint32_t Mode> Hkdf<ShaNid, Mode>::State::~State() {
  std::fill(Cache.begin(), Cache.end(), 0);
}

template class Hkdf<NID_sha256, EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY>;
template class Hkdf<NID_sha256, EVP_PKEY_HKDEF_MODE_EXPAND_ONLY>;
template class Hkdf<NID_sha512, EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY>;
template class Hkdf<NID_sha512, EVP_PKEY_HKDEF_MODE_EXPAND_ONLY>;

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
