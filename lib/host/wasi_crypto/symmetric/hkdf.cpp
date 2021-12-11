// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/hkdf.h"
#include "host/wasi_crypto/wrapper/random.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
HkdfSymmetricKey::HkdfSymmetricKey(SymmetricAlgorithm Alg,
                                   Span<uint8_t const> Raw)
    : Alg(Alg), Raw(Raw.begin(), Raw.end()) {}

WasiCryptoExpect<Span<const uint8_t>> HkdfSymmetricKey::raw() { return Raw; }

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
HkdfSymmetricState::import(SymmetricAlgorithm Alg,
                         std::optional<SymmetricKey> OptKey,
                         std::optional<SymmetricOptions> OptOptions) {
  if (!OptKey) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_KEY_REQUIRED);
  }

  if (auto Res = OptKey->isType<HkdfSymmetricKey>(); !Res) {
    return WasiCryptoUnexpect(Res);
  }

  auto Raw = OptKey->raw();
  if (!Raw) {
    return WasiCryptoUnexpect(Raw);
  }

  auto Ctx = Hkdf::make(Alg);
  if (!Ctx) {
    return WasiCryptoUnexpect(Ctx);
  }
  Ctx->setKey(*Raw);

  return std::unique_ptr<HkdfSymmetricState>{
      new HkdfSymmetricState{Alg, OptOptions, std::move(*Ctx)}};
}

WasiCryptoExpect<void> HkdfSymmetricState::absorb(Span<const uint8_t> Data) {
  return Ctx.absorb(Data);
}

WasiCryptoExpect<SymmetricKey>
HkdfSymmetricState::squeezeKey(SymmetricAlgorithm Alg) {
  auto Res = Ctx.squeezeKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SymmetricKey::import(Alg, *Res);
}

WasiCryptoExpect<void> HkdfSymmetricState::squeeze(Span<uint8_t> Out) {
  return Ctx.squeeze(Out);
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
    Hkdf Ctx)
    : SymmetricState::Base(Algorithm), OptOptions(OptOptions),
      Ctx(std::move(Ctx)) {}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
