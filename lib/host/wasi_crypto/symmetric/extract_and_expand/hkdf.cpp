// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/extract_and_expand/hkdf.h"
#include <openssl/rand.h>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

template <int Sha, int Mode>
WasiCryptoExpect<std::unique_ptr<Key>>
Hkdf<Sha, Mode>::KeyBuilder::generate(std::shared_ptr<Option>) {
  auto Len = keyLen();
  if (!Len) {
    return WasiCryptoUnexpect(Len);
  }

  std::vector<uint8_t> Raw(*Len, 0);
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
WasiCryptoExpect<__wasi_size_t> Hkdf<Sha, Mode>::KeyBuilder::keyLen() {
  switch (Alg) {
  case SymmetricAlgorithm::HkdfSha256Extract:
  case SymmetricAlgorithm::HkdfSha256Expand:
    return 32;
  case SymmetricAlgorithm::HkdfSha512Extract:
  case SymmetricAlgorithm::HkdfSha512Expand:
    return 64;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
  }
}

template <int Sha, int Mode>
WasiCryptoExpect<std::unique_ptr<typename Hkdf<Sha, Mode>::State>>
Hkdf<Sha, Mode>::State::open(std::shared_ptr<Key> OptKey,
                             std::shared_ptr<Option> OptOption) {
  ensureOrReturn(OptKey, __WASI_CRYPTO_ERRNO_KEY_REQUIRED);

  EVP_PKEY_CTX *Ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
  opensslAssuming(Ctx);
  opensslAssuming(EVP_PKEY_derive_init(Ctx));

  opensslAssuming(EVP_PKEY_CTX_set_hkdf_md(Ctx, ShaMap.at(Sha)));
  opensslAssuming(EVP_PKEY_CTX_hkdf_mode(Ctx, Mode));

  OptKey->inner().locked([&Ctx](auto &Inner) {
    opensslAssuming(
        EVP_PKEY_CTX_set1_hkdf_key(Ctx, Inner.Data.data(), Inner.Data.size()));
  });

  return std::make_unique<State>(OptOption, Ctx);
}

template <int Sha, int Mode>
WasiCryptoExpect<void>
Hkdf<Sha, Mode>::State::absorb(Span<const uint8_t> Data) {
  std::unique_lock Lock{Mutex};

  Cache.insert(Cache.end(), Data.begin(), Data.end());
  return {};
  //  switch (Alg) {
  //  case SymmetricAlgorithm::HkdfSha256Extract:
  //  case SymmetricAlgorithm::HkdfSha512Extract:
  //    ensureOrReturn(Cache, __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  //    opensslAssuming(
  //        EVP_PKEY_CTX_set1_hkdf_salt(Ctx.get(), Data.data(), Data.size()));
  //    return {};
  //  case SymmetricAlgorithm::HkdfSha256Expand:
  //  case SymmetricAlgorithm::HkdfSha512Expand:
  //    opensslAssuming(
  //        EVP_PKEY_CTX_add1_hkdf_info(Ctx.get(), Data.data(), Data.size()));
  //    return {};
  //  default:
  //    assumingUnreachable();
  //  }
}

template <int Sha, int Mode>
WasiCryptoExpect<std::unique_ptr<Key>>
Hkdf<Sha, Mode>::State::squeezeKey(SymmetricAlgorithm InputAlg) {
  ensureOrReturn(Mode == EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY,
                 __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  std::shared_lock Lock{Mutex};

  opensslAssuming(
      EVP_PKEY_CTX_set1_hkdf_salt(Ctx.get(), Cache.data(), Cache.size()));

  // check Size
  size_t Size;
  opensslAssuming(EVP_PKEY_derive(Ctx.get(), nullptr, &Size));

  std::vector<uint8_t> Data(Size);
  opensslAssuming(EVP_PKEY_derive(Ctx.get(), Data.data(), &Size));

  // TODO: may a better way
  return std::make_unique<Key>(InputAlg, std::move(Data));
}

template <int Sha, int Mode>
WasiCryptoExpect<void> Hkdf<Sha, Mode>::State::squeeze(Span<uint8_t> Out) {
  ensureOrReturn(Mode == EVP_PKEY_HKDEF_MODE_EXPAND_ONLY,
                 __WASI_CRYPTO_ERRNO_INVALID_OPERATION);

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

template class Hkdf<256, EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY>;
template class Hkdf<512, EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY>;
template class Hkdf<256, EVP_PKEY_HKDEF_MODE_EXPAND_ONLY>;
template class Hkdf<512, EVP_PKEY_HKDEF_MODE_EXPAND_ONLY>;

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
