// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/hmac_sha2.h"
#include "host/wasi_crypto/random.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

HmacSha2SymmetricKey::HmacSha2SymmetricKey(SymmetricAlgorithm Alg,
                                           Span<uint8_t const> Raw)
    : Alg(Alg), Raw(Raw.begin(), Raw.end()) {}

WasiCryptoExpect<Span<uint8_t>> HmacSha2SymmetricKey::raw() { return Raw; }

SymmetricAlgorithm HmacSha2SymmetricKey::alg() { return Alg; }

HmacSha2KeyBuilder::HmacSha2KeyBuilder(SymmetricAlgorithm Alg) : Alg{Alg} {}

WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
HmacSha2KeyBuilder::generate(std::shared_ptr<SymmetricOption>) {
  auto Len = keyLen();
  CryptoRandom Random;
  if (!Len) {
    return WasiCryptoUnexpect(Len);
  }
  std::vector<uint8_t> Raw(*Len, 0);
  if(auto Res = Random.fill(Raw); !Res.has_value()) {
    return WasiCryptoUnexpect(Res);
  };
  return import(Raw);
}

WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
HmacSha2KeyBuilder::import(Span<uint8_t const> Raw) {
  return std::make_unique<HmacSha2SymmetricKey>(Alg, Raw);
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
                             std::shared_ptr<SymmetricKey> OptKey,
                             std::shared_ptr<SymmetricOption> OptOptions) {
  if (OptKey == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_KEY_REQUIRED);
  }
  auto Key =
      std::dynamic_pointer_cast<HmacSha2SymmetricKey>(OptKey);
  if (Key == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }

  auto Raw = Key->raw();
  if (!Raw) {
    return WasiCryptoUnexpect(Raw);
  }

  return std::unique_ptr<HmacSha2SymmetricState>(
      new HmacSha2SymmetricState{Alg, *Raw, std::move(OptOptions)});
}

WasiCryptoExpect<void>
HmacSha2SymmetricState::absorb(Span<const uint8_t> Data) {
  HMAC_Update(Ctx.get(), Data.data(), Data.size());
  return {};
}

WasiCryptoExpect<SymmetricTag> HmacSha2SymmetricState::squeezeTag() {
  SymmetricTag Tag{Alg};
  auto Raw = Tag.raw();
  unsigned int Len;
  HMAC_Final(Ctx.get(), Raw.data(), &Len);
  return Tag;
}

HmacSha2SymmetricState::HmacSha2SymmetricState(
    SymmetricAlgorithm Alg, Span<uint8_t> Raw,
    std::shared_ptr<SymmetricOption> OptOptions)
    : SymmetricState(Alg, std::move(OptOptions)) {
  switch (Alg) {
  case SymmetricAlgorithm::HmacSha256:
    HMAC_Init_ex(Ctx.get(), Raw.data(), Raw.size(), EVP_sha256(), nullptr);
    break;
  case SymmetricAlgorithm::HmacSha512:
    HMAC_Init_ex(Ctx.get(), Raw.data(), Raw.size(), EVP_sha512(), nullptr);
    break;
  default: // never happen
    return;
  }
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
