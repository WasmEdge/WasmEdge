// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/hmac_sha2.h"
#include "host/wasi_crypto/random.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

HmacSha2SymmetricKey::HmacSha2SymmetricKey(SymmetricAlgorithm Alg,
                                           Span<uint8_t const> Raw)
    : Alg(Alg), Raw(Raw.begin(), Raw.end()) {}

HmacSha2SymmetricKey::~HmacSha2SymmetricKey() {
  std::fill(Raw.begin(), Raw.end(), 0);
}

WasiCryptoExpect<std::vector<uint8_t>> HmacSha2SymmetricKey::raw() {
  return Raw;
}

SymmetricAlgorithm HmacSha2SymmetricKey::alg() { return Alg; }

HmacSha2KeyBuilder::HmacSha2KeyBuilder(SymmetricAlgorithm Alg) : Alg{Alg} {}

WasiCryptoExpect<SymmetricKey>
HmacSha2KeyBuilder::generate(std::optional<SymmetricOptions>) {
  auto Len = keyLen();
  CryptoRandom Random;
  if (!Len) {
    return WasiCryptoUnexpect(Len);
  }
  std::vector<uint8_t> Raw(*Len, 0);
  if (auto Res = Random.fill(Raw); !Res.has_value()) {
    return WasiCryptoUnexpect(Res);
  }
  return import(Raw);
}

WasiCryptoExpect<SymmetricKey>
HmacSha2KeyBuilder::import(Span<uint8_t const> Raw) {
  return SymmetricKey{std::make_unique<HmacSha2SymmetricKey>(Alg, Raw)};
}

WasiCryptoExpect<__wasi_size_t> HmacSha2KeyBuilder::keyLen() {
  switch (Alg) {
  case SymmetricAlgorithm::HmacSha256:
    return 32;
  case SymmetricAlgorithm::HmacSha512:
    return 64;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
  }
}

WasiCryptoExpect<std::unique_ptr<HmacSha2SymmetricState>>
HmacSha2SymmetricState::make(SymmetricAlgorithm Alg,
                             std::optional<SymmetricKey> OptKey,
                             std::optional<SymmetricOptions> OptOptions) {
  if (!OptKey) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_KEY_REQUIRED);
  }
  if (auto Res = OptKey->isType<HmacSha2SymmetricKey>(); !Res) {
    return WasiCryptoUnexpect(Res);
  }

  OpenSSlUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx{EVP_MD_CTX_new()};
  if (Ctx == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  EVP_MD const *Md;
  switch (Alg) {
  case SymmetricAlgorithm::HmacSha256:
    Md = EVP_sha256();
    break;
  case SymmetricAlgorithm::HmacSha512:
    Md = EVP_sha512();
    break;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  }

  auto Raw = OptKey->raw();
  if (!Raw) {
    return WasiCryptoUnexpect(Raw);
  }

  OpenSSlUniquePtr<EVP_PKEY, EVP_PKEY_free> PKey{
      EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, nullptr, Raw->data(), Raw->size())};
  if (PKey == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  if (1 != EVP_DigestSignInit(Ctx.get(), nullptr, Md, nullptr, PKey.get())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return std::unique_ptr<HmacSha2SymmetricState>(
      new HmacSha2SymmetricState{Alg, OptOptions, std::move(Ctx)});
}

WasiCryptoExpect<std::vector<uint8_t>>
HmacSha2SymmetricState::optionsGet(std::string_view Name) {
  if (!OptOptions) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }
  return OptOptions->get(Name);
}

WasiCryptoExpect<uint64_t>
HmacSha2SymmetricState::optionsGetU64(std::string_view Name) {
  if (!OptOptions) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }
  return OptOptions->getU64(Name);
}

WasiCryptoExpect<void>
HmacSha2SymmetricState::absorb(Span<const uint8_t> Data) {
  if (1 != EVP_DigestSignUpdate(Ctx.get(), Data.data(), Data.size())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  return {};
}

WasiCryptoExpect<SymmetricTag> HmacSha2SymmetricState::squeezeTag() {
  auto CacheSize = EVP_MD_CTX_size(Ctx.get());
  uint8_t Cache[CacheSize];
  size_t ActualOutSize;

  if (1 != EVP_DigestSignFinal(Ctx.get(), Cache, &ActualOutSize)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  SymmetricTag Tag{Alg, {*&Cache, ActualOutSize}};
  return Tag;
}

HmacSha2SymmetricState::HmacSha2SymmetricState(
    SymmetricAlgorithm Alg, std::optional<SymmetricOptions> OptOptions,
    OpenSSlUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx)
    : SymmetricStateBase(Alg), OptOptions(OptOptions), Ctx(std::move(Ctx)) {}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
