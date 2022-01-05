// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/extract_and_expand/hkdf.h"
#include "host/wasi_crypto/wrapper/random.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

HkdfKeyBuilder::HkdfKeyBuilder(SymmetricAlgorithm Alg)
    : Alg(Alg) {}

WasiCryptoExpect<Key>
HkdfKeyBuilder::generate(std::optional<Options>) {
  auto Len = keyLen();
  if (!Len) {
    return WasiCryptoUnexpect(Len);
  }

  std::vector<uint8_t> Raw(*Len, 0);
  CryptoRandom Random;
  if (auto Res = Random.fill(Raw); !Res.has_value()) {
    return WasiCryptoUnexpect(Res);
  }

  return import(Raw);
}

WasiCryptoExpect<Key>
HkdfKeyBuilder::import(Span<uint8_t const> Raw) {
  return Key{std::make_unique<HkdfKey>(Alg, Raw)};
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
HkdfState::import(SymmetricAlgorithm Alg,
                           std::optional<Key> OptKey,
                           std::optional<Options> OptOptions) {
  if (!OptKey) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_KEY_REQUIRED);
  }

  if (auto Res = OptKey->isType<HkdfKey>(); !Res) {
    return WasiCryptoUnexpect(Res);
  }

  auto Ctx = OptKey->inner()->locked(
      [Alg](auto &Data) {   // init ctx
        OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx{
            EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr)};

        opensslAssuming(EVP_PKEY_derive_init(Ctx.get()));

        // init md and mode
        EVP_MD const *Md;
        int Mode;
        switch (Alg) {
        case SymmetricAlgorithm::HkdfSha256Extract:
          Md = EVP_sha256();
          Mode = EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY;
          break;
        case SymmetricAlgorithm::HkdfSha256Expand:
          Md = EVP_sha256();
          Mode = EVP_PKEY_HKDEF_MODE_EXPAND_ONLY;
          break;
        case SymmetricAlgorithm::HkdfSha512Extract:
          Md = EVP_sha512();
          Mode = EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY;
          break;
        case SymmetricAlgorithm::HkdfSha512Expand:
          Md = EVP_sha512();
          Mode = EVP_PKEY_HKDEF_MODE_EXPAND_ONLY;
          break;
        default:
          __builtin_unreachable();
        }

        opensslAssuming(EVP_PKEY_CTX_set_hkdf_md(Ctx.get(), Md));
        opensslAssuming(EVP_PKEY_CTX_hkdf_mode(Ctx.get(), Mode));

        opensslAssuming(EVP_PKEY_CTX_set1_hkdf_key(Ctx.get(), Key.data(), Key.size()));

        return HkdfCtx{Alg, std::move(Ctx)};
      });
  if (!Ctx) {
    return WasiCryptoUnexpect(Ctx);
  }

  return std::unique_ptr<HkdfState>{
      new HkdfState{Alg, OptOptions, std::move(*Ctx)}};
}

WasiCryptoExpect<void> HkdfState::absorb(Span<const uint8_t> Data) {
  switch (Alg) {
  case SymmetricAlgorithm::HkdfSha256Extract:
  case SymmetricAlgorithm::HkdfSha512Extract:
    opensslAssuming(EVP_PKEY_CTX_set1_hkdf_salt(Ctx.get(), Data.data(), Data.size()));
    return {};
  case SymmetricAlgorithm::HkdfSha256Expand:
  case SymmetricAlgorithm::HkdfSha512Expand:
    opensslAssuming(EVP_PKEY_CTX_add1_hkdf_info(Ctx.get(), Data.data(), Data.size()));
    return {};
  default:
    __builtin_unreachable();
  }
}

WasiCryptoExpect<Key>
HkdfState::squeezeKey(SymmetricAlgorithm Alg) {
  // check Size
  size_t Size;
  opensslAssuming(EVP_PKEY_derive(Ctx.get(), nullptr, &Size));

  // allocate
  std::vector<uint8_t> Data;
  Data.reserve(Size);
  Data.resize(Size);

  opensslAssuming(EVP_PKEY_derive(Ctx.get(), Data.data(), &Size));

  return Data;
}

WasiCryptoExpect<void> HkdfState::squeeze(Span<uint8_t> Out) {
  size_t Size = Out.size();
  opensslAssuming(EVP_PKEY_derive(Ctx.get(), Out.data(), &Size));
  return {};
}

WasiCryptoExpect<std::vector<uint8_t>>
HkdfState::optionsGet(std::string_view Name) {
  if (!OptOptions) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }
  return OptOptions->inner()->locked(
      [&Name](auto &Inner) { return Inner.get(Name); });
}

WasiCryptoExpect<uint64_t>
HkdfState::optionsGetU64(std::string_view Name) {
  if (!OptOptions) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }
  return OptOptions->inner()->locked(
      [&Name](auto &Inner) { return Inner.getU64(Name); });
}


} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
