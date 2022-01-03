// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/charcha_poly.h"
#include "host/wasi_crypto/wrapper/random.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

ChaChaPolySymmetricKey::ChaChaPolySymmetricKey(SymmetricAlgorithm Alg,
                                               Span<uint8_t const> Raw)
    : Alg(Alg), Raw(Raw.begin(), Raw.end()) {}

ChaChaPolySymmetricKeyBuilder::ChaChaPolySymmetricKeyBuilder(
    SymmetricAlgorithm Alg)
    : Alg{Alg} {}

WasiCryptoExpect<SymmetricKey>
ChaChaPolySymmetricKeyBuilder::generate(std::optional<SymmetricOptions>) {
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
ChaChaPolySymmetricKeyBuilder::import(Span<uint8_t const> Raw) {
  return SymmetricKey{std::make_unique<ChaChaPolySymmetricKey>(Alg, Raw)};
}

WasiCryptoExpect<__wasi_size_t> ChaChaPolySymmetricKeyBuilder::keyLen() {
  switch (Alg) {
  case SymmetricAlgorithm::ChaCha20Poly1305:
    return 16;
  case SymmetricAlgorithm::XChaCha20Poly1305:
    return 32;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
  }
}

WasiCryptoExpect<std::unique_ptr<ChaChaPolySymmetricState>>
ChaChaPolySymmetricState::make(SymmetricAlgorithm, std::optional<SymmetricKey>,
                               std::optional<SymmetricOptions>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::vector<uint8_t>>
ChaChaPolySymmetricState::optionsGet(std::string_view Name) {
  return Options.get(Name);
}

WasiCryptoExpect<uint64_t>
ChaChaPolySymmetricState::optionsGetU64(std::string_view Name) {
  return Options.getU64(Name);
}

WasiCryptoExpect<void> ChaChaPolySymmetricState::absorb(Span<const uint8_t> Data) {
  return Ctx.absorb(Data);
}

WasiCryptoExpect<__wasi_size_t>
ChaChaPolySymmetricState::encryptUnchecked(Span<uint8_t> Out,
                                       Span<const uint8_t> Data) {
  auto Tag = Ctx.encryptDetached(Out.first(Data.size()), Data);
  if (!Tag) {
    return WasiCryptoUnexpect(Tag);
  }

  std::copy(Tag->begin(), Tag->end(), Out.subspan(Data.size()).begin());

  return Out.size();
}

WasiCryptoExpect<SymmetricTag>
ChaChaPolySymmetricState::encryptDetachedUnchecked(Span<uint8_t> Out,
                                               Span<const uint8_t> Data) {
  auto Res = Ctx.encryptDetached(Out, Data);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SymmetricTag{Alg, std::move(*Res)};
}

WasiCryptoExpect<__wasi_size_t>
ChaChaPolySymmetricState::decryptUnchecked(Span<uint8_t> Out,
                                       Span<uint8_t const> Data) {
  return decryptDetachedUnchecked(Out, Data.first(Out.size()),
                                  Data.subspan(Out.size()));
}

WasiCryptoExpect<__wasi_size_t> ChaChaPolySymmetricState::decryptDetachedUnchecked(
    Span<uint8_t> Out, Span<const uint8_t> Data, Span<uint8_t const> RawTag) {
  return Ctx.decryptDetached(Out, Data, RawTag);
}


} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
