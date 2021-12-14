// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/hmac_sha2.h"
#include "host/wasi_crypto/wrapper/random.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

HmacSha2SymmetricKey::HmacSha2SymmetricKey(SymmetricAlgorithm Alg,
                                           Span<uint8_t const> Raw)
    : Alg(Alg), Raw(Raw.begin(), Raw.end()) {}

HmacSha2SymmetricKey::~HmacSha2SymmetricKey() {
  std::fill(Raw.begin(), Raw.end(), 0);
}

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
HmacSha2SymmetricState::import(SymmetricAlgorithm Alg,
                               std::optional<SymmetricKey> OptKey,
                               std::optional<SymmetricOptions> OptOptions) {
  if (!OptKey) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_KEY_REQUIRED);
  }

  if (auto Res = OptKey->isType<HmacSha2SymmetricKey>(); !Res) {
    return WasiCryptoUnexpect(Res);
  }

  auto Ctx = OptKey->inner()->locked(
      [Alg](auto &Inner) -> WasiCryptoExpect<HmacSha2Ctx> {
        return HmacSha2Ctx::import(Alg, Inner->asRef());
      });
  if (!Ctx) {
    return WasiCryptoUnexpect(Ctx);
  }

  return std::unique_ptr<HmacSha2SymmetricState>(
      new HmacSha2SymmetricState{Alg, OptOptions, std::move(*Ctx)});
}

WasiCryptoExpect<std::vector<uint8_t>>
HmacSha2SymmetricState::optionsGet(std::string_view Name) {
  if (!OptOptions) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }
  return OptOptions->inner()->locked(
      [&Name](auto &Inner) { return Inner.get(Name); });
}

WasiCryptoExpect<uint64_t>
HmacSha2SymmetricState::optionsGetU64(std::string_view Name) {
  if (!OptOptions) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }
  return OptOptions->inner()->locked(
      [&Name](auto &Inner) { return Inner.getU64(Name); });
}

WasiCryptoExpect<void>
HmacSha2SymmetricState::absorb(Span<const uint8_t> Data) {
  return Ctx.absorb(Data);
}

WasiCryptoExpect<SymmetricTag> HmacSha2SymmetricState::squeezeTag() {
  auto Res = Ctx.squeezeTag();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  SymmetricTag Tag{Alg, std::move(*Res)};
  return Tag;
}

HmacSha2SymmetricState::HmacSha2SymmetricState(
    SymmetricAlgorithm Alg, std::optional<SymmetricOptions> OptOptions,
    HmacSha2Ctx Ctx)
    : SymmetricState::Base(Alg), OptOptions(OptOptions), Ctx(std::move(Ctx)) {}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
