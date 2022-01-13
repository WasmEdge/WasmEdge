// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/extract_and_expand/hkdf.h"
#include "openssl/rand.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

template <int Sha, int Mode>
WasiCryptoExpect<std::unique_ptr<Key>>
Hkdf<Sha, Mode>::KeyBuilder::generate(std::shared_ptr<Options>) {
  std::vector<uint8_t> Raw(keyLen(), 0);
  RAND_bytes(Raw.data(), Raw.size());

  return std::make_unique<Key>(Alg, std::move(Raw));
}

template <int Sha, int Mode>
WasiCryptoExpect<std::unique_ptr<Key>>
Hkdf<Sha, Mode>::KeyBuilder::import(Span<uint8_t const> Raw) {
  return std::make_unique<Key>(Alg,
                               std::vector<uint8_t>{Raw.begin(), Raw.end()});
}

template <int Sha, int Mode>
__wasi_size_t Hkdf<Sha, Mode>::KeyBuilder::keyLen() {
  return Sha / 8;
}

template <int Sha, int Mode>
WasiCryptoExpect<std::unique_ptr<typename Hkdf<Sha, Mode>::State>>
Hkdf<Sha, Mode>::State::open(std::shared_ptr<Key> OptKey,
                             std::shared_ptr<Options> OptOption) {
  ensureOrReturn(OptKey, __WASI_CRYPTO_ERRNO_KEY_REQUIRED);

  EVP_PKEY_CTX *Ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
  opensslAssuming(Ctx);
  opensslAssuming(EVP_PKEY_derive_init(Ctx));

  opensslAssuming(EVP_PKEY_CTX_set_hkdf_md(Ctx, ShaMap.at(Sha)));
  opensslAssuming(EVP_PKEY_CTX_hkdf_mode(Ctx, Mode));

  opensslAssuming(EVP_PKEY_CTX_set1_hkdf_key(Ctx, OptKey->data().data(),
                                             OptKey->data().size()));

  return std::make_unique<State>(OptOption, Ctx);
}

template <int Sha, int Mode>
WasiCryptoExpect<void>
Hkdf<Sha, Mode>::State::absorb(Span<const uint8_t> Data) {
  std::unique_lock<std::shared_mutex> Lock{Mutex};

  Cache.insert(Cache.end(), Data.begin(), Data.end());
  return {};
}

template <int Sha, int Mode>
WasiCryptoExpect<std::unique_ptr<Key>>
Hkdf<Sha, Mode>::State::squeezeKey(SymmetricAlgorithm InputAlg) {
  if constexpr (Mode != EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  std::shared_lock<std::shared_mutex> Lock{Mutex};

  opensslAssuming(
      EVP_PKEY_CTX_set1_hkdf_salt(Ctx.get(), Cache.data(), Cache.size()));

  // check Size
  size_t Size;
  opensslAssuming(EVP_PKEY_derive(Ctx.get(), nullptr, &Size));

  std::vector<uint8_t> Data(Size);
  opensslAssuming(EVP_PKEY_derive(Ctx.get(), Data.data(), &Size));

  return std::make_unique<Key>(InputAlg, std::move(Data));
}

template <int Sha, int Mode>
WasiCryptoExpect<void> Hkdf<Sha, Mode>::State::squeeze(Span<uint8_t> Out) {
  if constexpr (Mode != EVP_PKEY_HKDEF_MODE_EXPAND_ONLY) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  opensslAssuming(
      EVP_PKEY_CTX_add1_hkdf_info(Ctx.get(), Cache.data(), Cache.size()));

  // TODO : More check about error
  size_t Size = Out.size();
  ensureOrReturn(EVP_PKEY_derive(Ctx.get(), Out.data(), &Size),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return {};
}

template <int Sha, int Mode>
WasiCryptoExpect<std::vector<uint8_t>>
Hkdf<Sha, Mode>::State::optionsGet(std::string_view Name) {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->get(Name);
}

template <int Sha, int Mode>
WasiCryptoExpect<uint64_t>
Hkdf<Sha, Mode>::State::optionsGetU64(std::string_view Name) {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->getU64(Name);
}

template <int Sha, int Mode> Hkdf<Sha, Mode>::State::~State() {
  std::fill(Cache.begin(), Cache.end(), 0);
}

template class Hkdf<256, EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY>;
template class Hkdf<512, EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY>;
template class Hkdf<256, EVP_PKEY_HKDEF_MODE_EXPAND_ONLY>;
template class Hkdf<512, EVP_PKEY_HKDEF_MODE_EXPAND_ONLY>;

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
