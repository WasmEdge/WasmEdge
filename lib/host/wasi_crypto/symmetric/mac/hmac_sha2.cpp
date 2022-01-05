// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/mac/hmac_sha2.h"
#include "host/wasi_crypto/wrapper/random.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

HmacSha2Key::HmacSha2Key(SymmetricAlgorithm Alg,
                                           Span<uint8_t const> Raw)
    : Alg(Alg), Raw(Raw.begin(), Raw.end()) {}

HmacSha2Key::~HmacSha2Key() {
  std::fill(Raw.begin(), Raw.end(), 0);
}

HmacSha2KeyBuilder::HmacSha2KeyBuilder(SymmetricAlgorithm Alg) : Alg{Alg} {}

WasiCryptoExpect<Key>
HmacSha2KeyBuilder::generate(std::optional<Options>) {
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

WasiCryptoExpect<Key>
HmacSha2KeyBuilder::import(Span<uint8_t const> Raw) {
  return Key{std::make_unique<Key>(Alg, Raw)};
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

WasiCryptoExpect<std::unique_ptr<HmacSha2State>>
HmacSha2State::import(SymmetricAlgorithm Alg,
                      std::shared_ptr<Key> OptKey,
                      std::shared_ptr<Options> OptOptions) {
  if (!OptKey) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_KEY_REQUIRED);
  }

  if (auto Res = OptKey->isType<HmacSha2Key>(); !Res) {
    return WasiCryptoUnexpect(Res);
  }

  auto Ctx = OptKey->inner()->locked([Alg](auto &Inner) {
    OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> PKey{
        EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, nullptr, Raw.data(), Raw.size())};
    opensslAssuming(PKey);

    OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx{EVP_MD_CTX_new()};
    opensslAssuming(Ctx);

    EVP_MD const *Md;
    switch (Alg) {
    case SymmetricAlgorithm::HmacSha256:
      Md = EVP_sha256();
      break;
    case SymmetricAlgorithm::HmacSha512:
      Md = EVP_sha512();
      break;
    default:
      __builtin_unreachable();
    }

    opensslAssuming(
        EVP_DigestSignInit(Ctx.get(), nullptr, Md, nullptr, PKey.get()));

    return HmacSha2Ctx{std::move(Ctx)};
  });
  if (!Ctx) {
    return WasiCryptoUnexpect(Ctx);
  }

  return std::unique_ptr<HmacSha2State>(
      new HmacSha2State{Alg, OptOptions, std::move(*Ctx)});
}

WasiCryptoExpect<std::vector<uint8_t>>
HmacSha2State::optionsGet(std::string_view Name) {
  if (!OptOptions) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }
  return OptOptions->inner()->locked(
      [&Name](auto &Inner) { return Inner.get(Name); });
}

WasiCryptoExpect<uint64_t> HmacSha2State::optionsGetU64(std::string_view Name) {
  if (!OptOptions) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }
  return OptOptions->inner()->locked(
      [&Name](auto &Inner) { return Inner.getU64(Name); });
}

WasiCryptoExpect<void> HmacSha2State::absorb(Span<const uint8_t> Data) {
  opensslAssuming(EVP_DigestSignUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

WasiCryptoExpect<Tag> HmacSha2State::squeezeTag() {
  {
    size_t ActualOutSize;
    opensslAssuming(EVP_DigestSignFinal(Ctx.get(), nullptr, &ActualOutSize));

    std::vector<uint8_t> Res;
    Res.reserve(ActualOutSize);
    Res.resize(ActualOutSize);

    opensslAssuming(EVP_DigestSignFinal(Ctx.get(), Res.data(), &ActualOutSize));

    return Res;
  }
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  Tag Tag{Alg, std::move(*Res)};
  return Tag;
}


} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
