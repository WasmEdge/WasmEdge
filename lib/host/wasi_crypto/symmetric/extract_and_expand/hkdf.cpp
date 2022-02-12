// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/extract_and_expand/hkdf.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/evpwrapper.h"
#include "host/wasi_crypto/util.h"
#include "wasi_crypto/api.hpp"

#include "openssl/rand.h"
namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

template <int ShaNid>
WasiCryptoExpect<std::unique_ptr<Key>>
Hkdf<ShaNid>::Extract::KeyBuilder::generate(std::shared_ptr<Options>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
}

template <int ShaNid>
WasiCryptoExpect<std::unique_ptr<Key>>
Hkdf<ShaNid>::Extract::KeyBuilder::import(Span<uint8_t const> Raw) {
  return std::make_unique<Key>(Alg,
                               std::vector<uint8_t>{Raw.begin(), Raw.end()});
}

template <int ShaNid>
WasiCryptoExpect<std::unique_ptr<typename Hkdf<ShaNid>::Extract::State>>
Hkdf<ShaNid>::Extract::State::open(std::shared_ptr<Key> OptKey,
                                   std::shared_ptr<Options> OptOption) {
  ensureOrReturn(OptKey, __WASI_CRYPTO_ERRNO_KEY_REQUIRED);

  EvpPkeyCtxPtr Ctx{EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr)};
  opensslAssuming(EVP_PKEY_derive_init(Ctx.get()));
  opensslAssuming(EVP_PKEY_CTX_set_hkdf_md(Ctx.get(), getShaCtx()));
  opensslAssuming(
      EVP_PKEY_CTX_hkdf_mode(Ctx.get(), EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY));
  opensslAssuming(EVP_PKEY_CTX_set1_hkdf_key(Ctx.get(), OptKey->data().data(),
                                             OptKey->data().size()));

  return std::make_unique<State>(OptOption, std::move(Ctx));
}

template <int ShaNid>
WasiCryptoExpect<void>
Hkdf<ShaNid>::Extract::State::absorb(Span<const uint8_t> Data) {
  std::unique_lock<std::shared_mutex> Lock{Mutex};
  Salt.insert(Salt.end(), Data.begin(), Data.end());
  return {};
}

template <int ShaNid>
WasiCryptoExpect<std::unique_ptr<Key>>
Hkdf<ShaNid>::Extract::State::squeezeKey(SymmetricAlgorithm InputAlg) {
  std::shared_lock<std::shared_mutex> Lock{Mutex};

  opensslAssuming(
      EVP_PKEY_CTX_set1_hkdf_salt(Ctx.get(), Salt.data(), Salt.size()));

  std::vector<uint8_t> Data(getKeySize());
  size_t Size;
  opensslAssuming(EVP_PKEY_derive(Ctx.get(), Data.data(), &Size));
  ensureOrReturn(Size == getKeySize(), __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return std::make_unique<Key>(InputAlg, std::move(Data));
}

template <int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Hkdf<ShaNid>::Extract::State::optionsGet(std::string_view Name) {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->get(Name);
}

template <int ShaNid>
WasiCryptoExpect<uint64_t>
Hkdf<ShaNid>::Extract::State::optionsGetU64(std::string_view Name) {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->getU64(Name);
}

template <int ShaNid>
WasiCryptoExpect<std::unique_ptr<Key>>
Hkdf<ShaNid>::Expand::KeyBuilder::generate(std::shared_ptr<Options>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
}

template <int ShaNid>
WasiCryptoExpect<std::unique_ptr<Key>>
Hkdf<ShaNid>::Expand::KeyBuilder::import(Span<uint8_t const> Raw) {
  ensureOrReturn(Raw.size() == getKeySize(), __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return std::make_unique<Key>(Alg,
                               std::vector<uint8_t>{Raw.begin(), Raw.end()});
}

template <int ShaNid>
WasiCryptoExpect<std::unique_ptr<typename Hkdf<ShaNid>::Expand::State>>
Hkdf<ShaNid>::Expand::State::open(std::shared_ptr<Key> OptKey,
                                  std::shared_ptr<Options> OptOption) {
  ensureOrReturn(OptKey, __WASI_CRYPTO_ERRNO_KEY_REQUIRED);

  EvpPkeyCtxPtr Ctx{EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr)};
  opensslAssuming(EVP_PKEY_derive_init(Ctx.get()));
  opensslAssuming(EVP_PKEY_CTX_set_hkdf_md(Ctx.get(), getShaCtx()));
  opensslAssuming(
      EVP_PKEY_CTX_hkdf_mode(Ctx.get(), EVP_PKEY_HKDEF_MODE_EXPAND_ONLY));
  opensslAssuming(EVP_PKEY_CTX_set1_hkdf_key(Ctx.get(), OptKey->data().data(),
                                             OptKey->data().size()));

  return std::make_unique<State>(OptOption, std::move(Ctx));
}

template <int ShaNid>
WasiCryptoExpect<void>
Hkdf<ShaNid>::Expand::State::absorb(Span<const uint8_t> Data) {
  opensslAssuming(
      EVP_PKEY_CTX_add1_hkdf_info(Ctx.get(), Data.data(), Data.size()));
  return {};
}

template <int ShaNid>
WasiCryptoExpect<void> Hkdf<ShaNid>::Expand::State::squeeze(Span<uint8_t> Out) {
  ensureOrReturn(
      EVP_PKEY_derive(Ctx.get(), Out.data(), addressOfTempory(Out.size())),
      __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return {};
}

template <int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Hkdf<ShaNid>::Expand::State::optionsGet(std::string_view Name) {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->get(Name);
}

template <int ShaNid>
WasiCryptoExpect<uint64_t>
Hkdf<ShaNid>::Expand::State::optionsGetU64(std::string_view Name) {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->getU64(Name);
}

template <int ShaNid> constexpr uint32_t Hkdf<ShaNid>::getKeySize() {
  if constexpr (ShaNid == NID_sha256)
    return 32;
  if constexpr (ShaNid == NID_sha512)
    return 64;
}

template <int ShaNid> constexpr void *Hkdf<ShaNid>::getShaCtx() {
  return static_cast<void *>(const_cast<EVP_MD *>(EVP_get_digestbynid(ShaNid)));
}

template class Hkdf<NID_sha256>::Extract;
template class Hkdf<NID_sha512>::Extract;
template class Hkdf<NID_sha256>::Expand;
template class Hkdf<NID_sha512>::Expand;

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
