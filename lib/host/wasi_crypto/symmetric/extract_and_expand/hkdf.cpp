// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/extract_and_expand/hkdf.h"
#include <openssl/kdf.h>
#include <openssl/rand.h>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {
namespace {

constexpr std::tuple<const EVP_MD *, int> getConfig(SymmetricAlgorithm Alg) {
  switch (Alg) {
  case SymmetricAlgorithm::HkdfSha256Extract:
    return {EVP_sha256(), EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY};
  case SymmetricAlgorithm::HkdfSha256Expand:
    return {EVP_sha256(), EVP_PKEY_HKDEF_MODE_EXPAND_ONLY};
  case SymmetricAlgorithm::HkdfSha512Extract:
    return {EVP_sha512(), EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY};
  case SymmetricAlgorithm::HkdfSha512Expand:
    return {EVP_sha512(), EVP_PKEY_HKDEF_MODE_EXPAND_ONLY};
  default:
    __builtin_unreachable();
  }
}
} // namespace

WasiCryptoExpect<std::unique_ptr<Key>>
HkdfKeyBuilder::generate(std::shared_ptr<Option>) {
  auto Len = keyLen();
  if (!Len) {
    return WasiCryptoUnexpect(Len);
  }

  std::vector<uint8_t> Raw(*Len, 0);
  RAND_bytes(Raw.data(), Raw.size());

  return std::make_unique<Key>(Alg, std::move(Raw));
}

WasiCryptoExpect<std::unique_ptr<Key>>
HkdfKeyBuilder::import(Span<uint8_t const> Raw) {
  return std::make_unique<Key>(Alg,
                               std::vector<uint8_t>{Raw.begin(), Raw.end()});
}

WasiCryptoExpect<__wasi_size_t> HkdfKeyBuilder::keyLen() {
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

WasiCryptoExpect<std::unique_ptr<HkdfState>>
HkdfState::open(SymmetricAlgorithm Alg, std::shared_ptr<Key> OptKey,
                std::shared_ptr<Option> OptOption) {
  ensureOrReturn(OptKey, __WASI_CRYPTO_ERRNO_KEY_REQUIRED);

  EVP_PKEY_CTX *Ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
  opensslAssuming(Ctx);
  opensslAssuming(EVP_PKEY_derive_init(Ctx));

  auto [Md, Mode] = getConfig(Alg);
  opensslAssuming(EVP_PKEY_CTX_set_hkdf_md(Ctx, Md));
  opensslAssuming(EVP_PKEY_CTX_hkdf_mode(Ctx, Mode));

  OptKey->inner().locked([&Ctx](auto &Inner) {
    opensslAssuming(
        EVP_PKEY_CTX_set1_hkdf_key(Ctx, Inner.Data.data(), Inner.Data.size()));
  });

  return std::make_unique<HkdfState>(OptOption, Ctx);
}

WasiCryptoExpect<void> HkdfState::absorb(Span<const uint8_t> Data) {
  switch (Alg) {
  case SymmetricAlgorithm::HkdfSha256Extract:
  case SymmetricAlgorithm::HkdfSha512Extract:
    opensslAssuming(
        EVP_PKEY_CTX_set1_hkdf_salt(Ctx.get(), Data.data(), Data.size()));
    return {};
  case SymmetricAlgorithm::HkdfSha256Expand:
  case SymmetricAlgorithm::HkdfSha512Expand:
    opensslAssuming(
        EVP_PKEY_CTX_add1_hkdf_info(Ctx.get(), Data.data(), Data.size()));
    return {};
  default:
    __builtin_unreachable();
  }
}

WasiCryptoExpect<std::unique_ptr<Key>>
HkdfState::squeezeKey(SymmetricAlgorithm Alg) {
  switch (Alg) {
  case SymmetricAlgorithm::HkdfSha256Extract:
    break;
  case SymmetricAlgorithm::HkdfSha512Extract:
    break;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  // check Size
  size_t Size;
  opensslAssuming(EVP_PKEY_derive(Ctx.get(), nullptr, &Size));

  std::vector<uint8_t> Data(Size);
  opensslAssuming(EVP_PKEY_derive(Ctx.get(), Data.data(), &Size));

  // TODO: may a better way
  return std::make_unique<Key>(Alg, std::move(Data));
}

WasiCryptoExpect<void> HkdfState::squeeze(Span<uint8_t> Out) {
  size_t Size = Out.size();
  ensureOrReturn(EVP_PKEY_derive(Ctx.get(), Out.data(), &Size),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
}

WasiCryptoExpect<std::vector<uint8_t>>
HkdfState::optionsGet(std::string_view Name) {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->get(Name);
}

WasiCryptoExpect<uint64_t> HkdfState::optionsGetU64(std::string_view Name) {
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOption->getU64(Name);
}

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
