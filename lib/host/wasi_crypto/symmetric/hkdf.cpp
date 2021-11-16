// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/hkdf.h"
#include "host/wasi_crypto/random.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
HkdfSymmetricKey::HkdfSymmetricKey(SymmetricAlgorithm Alg,
                                   Span<uint8_t const> Raw)
    : Alg(Alg), Raw(Raw.begin(), Raw.end()) {}

WasiCryptoExpect<std::vector<uint8_t>> HkdfSymmetricKey::raw() { return Raw; }

SymmetricAlgorithm HkdfSymmetricKey::alg() { return Alg; }

HkdfSymmetricKeyBuilder::HkdfSymmetricKeyBuilder(SymmetricAlgorithm Alg)
    : Alg(Alg) {}

WasiCryptoExpect<SymmetricKey>
HkdfSymmetricKeyBuilder::generate(std::optional<SymmetricOptions>) {
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

WasiCryptoExpect<SymmetricKey>
HkdfSymmetricKeyBuilder::import(Span<uint8_t const> Raw) {
  return SymmetricKey{std::make_unique<HkdfSymmetricKey>(Alg, Raw)};
}

WasiCryptoExpect<__wasi_size_t> HkdfSymmetricKeyBuilder::keyLen() {
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

WasiCryptoExpect<std::unique_ptr<HkdfSymmetricState>>
HkdfSymmetricState::make(SymmetricAlgorithm Alg,
                         std::optional<SymmetricKey> OptKey,
                         std::optional<SymmetricOptions> OptOptions) {
  if (!OptKey) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_KEY_REQUIRED);
  }

  if(auto Res = OptKey->isType<HkdfSymmetricKey>(); !Res) {
    return WasiCryptoUnexpect(Res);
  }

  auto Raw = OptKey->raw();
  if (!Raw) {
    return WasiCryptoUnexpect(Raw);
  }

  // init ctx
  OpenSSlUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx{
      EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr)};
  if (Ctx == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  if (EVP_PKEY_derive_init(Ctx.get()) <= 0) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

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
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
  }
  if (EVP_PKEY_CTX_set_hkdf_md(Ctx.get(), Md) <= 0) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  if (EVP_PKEY_CTX_hkdf_mode(Ctx.get(), Mode) <= 0) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  if (EVP_PKEY_CTX_set1_hkdf_key(Ctx.get(), Raw->data(), Raw->size()) <= 0) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  // init options
  //  if (OptOptions != nullptr) {
  //    auto Data = OptOptions->Inner.lock();
  //    if (auto &Salt = Data->Salt; Salt) {
  //      if (EVP_PKEY_CTX_set1_hkdf_salt(Ctx.get(), Salt->data(), Salt->size())
  //      <=
  //          0) {
  //        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  //      }
  //    }
  //  }

  return std::unique_ptr<HkdfSymmetricState>{
      new HkdfSymmetricState{Alg, OptOptions, std::move(Ctx)}};
}

WasiCryptoExpect<void> HkdfSymmetricState::absorb(Span<const uint8_t> Data) {
  switch (Alg) {
  case SymmetricAlgorithm::HkdfSha256Extract:
  case SymmetricAlgorithm::HkdfSha512Extract:
    if (EVP_PKEY_CTX_set1_hkdf_salt(Ctx.get(), Data.data(), Data.size()) <= 0) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
    }
    break;
  case SymmetricAlgorithm::HkdfSha256Expand:
  case SymmetricAlgorithm::HkdfSha512Expand:
    if (EVP_PKEY_CTX_add1_hkdf_info(Ctx.get(), Data.data(), Data.size()) <= 0) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
    }
    break;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
  return {};
}

WasiCryptoExpect<SymmetricKey>
HkdfSymmetricState::squeezeKey(SymmetricAlgorithm Alg) {
  // check Size
  size_t Size;
  if (EVP_PKEY_derive(Ctx.get(), nullptr, &Size) <= 0) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  // allocate
  uint8_t Data[Size];
  size_t NewSize;
  if (EVP_PKEY_derive(Ctx.get(), Data, &NewSize) <= 0) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  // check
  if (NewSize != Size) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return SymmetricKey::import(Alg, {*&Data, NewSize});
}

WasiCryptoExpect<void> HkdfSymmetricState::squeeze(Span<uint8_t> Out) {
  // check Size
  size_t Size;
  //  if (EVP_PKEY_derive(Ctx.get(), nullptr, &Size) <= 0) {
  //    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  //  }
  //
  //  if (Out.size() < Size) {
  //    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OVERFLOW);
  //  }

  if (EVP_PKEY_derive(Ctx.get(), Out.data(), &Size) <= 0) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  return {};
}

WasiCryptoExpect<std::vector<uint8_t>>
HkdfSymmetricState::optionsGet(std::string_view Name) {
  if (!OptOptions) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }
  return OptOptions->get(Name);
}

WasiCryptoExpect<uint64_t>
HkdfSymmetricState::optionsGetU64(std::string_view Name) {
  if (!OptOptions) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }
  return OptOptions->getU64(Name);
}

HkdfSymmetricState::HkdfSymmetricState(
    SymmetricAlgorithm Algorithm, std::optional<SymmetricOptions> OptOptions,
    OpenSSlUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx)
    : SymmetricStateBase(Algorithm), OptOptions(OptOptions),
      Ctx(std::move(Ctx)) {}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
