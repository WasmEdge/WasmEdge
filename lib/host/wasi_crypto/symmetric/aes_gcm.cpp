// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/aes_gcm.h"
#include "host/wasi_crypto/wrapper/aes_gcm.h"
#include "host/wasi_crypto/wrapper/random.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

AesGcmSymmetricKey::AesGcmSymmetricKey(SymmetricAlgorithm Alg,
                                       Span<uint8_t const> Raw)
    : Alg(Alg), Raw(Raw.begin(), Raw.end()) {}

WasiCryptoExpect<Span<const uint8_t>> AesGcmSymmetricKey::raw() { return Raw; }

SymmetricAlgorithm AesGcmSymmetricKey::alg() { return Alg; }

AesGcmSymmetricKeyBuilder::AesGcmSymmetricKeyBuilder(SymmetricAlgorithm Alg)
    : Alg(Alg) {}

WasiCryptoExpect<SymmetricKey> AesGcmSymmetricKeyBuilder::generate(
    std::optional<SymmetricOptions> OptOptions) {
  auto Len = keyLen();
  if (!Len) {
    return WasiCryptoUnexpect(Len);
  }

  auto Nonce = OptOptions->get("nonce");
  if (Nonce) {
    // but size not equal
    if (Nonce->size() != *Len) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_NONCE);
    }

    return import(*Nonce);
  }

  // generate random by host TODO: may I need to generate a option and register.
  // Need read proposal more detailed.
  std::vector<uint8_t> Raw(*Len, 0);
  CryptoRandom Random;
  if (auto Res = Random.fill(Raw); !Res) {
    return WasiCryptoUnexpect(Res);
  }

  return import(Raw);
}

WasiCryptoExpect<SymmetricKey>
AesGcmSymmetricKeyBuilder::import(Span<uint8_t const> Raw) {
  return SymmetricKey{std::make_unique<AesGcmSymmetricKey>(Alg, Raw)};
}

WasiCryptoExpect<__wasi_size_t> AesGcmSymmetricKeyBuilder::keyLen() {
  switch (Alg) {
  case SymmetricAlgorithm::Aes128Gcm:
    return 16;
  case SymmetricAlgorithm::Aes256Gcm:
    return 32;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
  }
}

WasiCryptoExpect<std::unique_ptr<AesGcmSymmetricState>>
AesGcmSymmetricState::make(SymmetricAlgorithm Alg,
                           std::optional<SymmetricKey> OptKey,
                           std::optional<SymmetricOptions> OptOptions) {
  if (OptKey) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_KEY_REQUIRED);
  }

  if (!OptOptions) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NONCE_REQUIRED);
  }

  if (auto Res = OptKey->isType<AesGcmSymmetricKey>(); !Res) {
    return WasiCryptoUnexpect(Res);
  }

  // init get Key data
  auto Raw = OptKey->raw();
  if (!Raw) {
    return WasiCryptoUnexpect(Raw);
  }

  // set key
  auto Ctx = AesGcm::make(Alg);
  if (!Ctx) {
    return WasiCryptoUnexpect(Ctx);
  }
  Ctx->setKey(*Raw);

  // set nonce
  auto Nonce = OptOptions->get("nonce");
  if (!Nonce) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NONCE_REQUIRED);
  }

  Ctx->setNonce(*Nonce);

  // new ptr
  return std::unique_ptr<AesGcmSymmetricState>{
      new AesGcmSymmetricState{Alg, *OptOptions, std::move(*Ctx)}};
}

WasiCryptoExpect<std::vector<uint8_t>>
AesGcmSymmetricState::optionsGet(std::string_view Name) {
  return Options.get(Name);
}

WasiCryptoExpect<uint64_t>
AesGcmSymmetricState::optionsGetU64(std::string_view Name) {
  return Options.getU64(Name);
}

WasiCryptoExpect<void> AesGcmSymmetricState::absorb(Span<const uint8_t> Data) {
  return Ctx.absorb(Data);
}

WasiCryptoExpect<__wasi_size_t>
AesGcmSymmetricState::encryptUnchecked(Span<uint8_t> Out,
                                       Span<const uint8_t> Data) {

  auto Tag = Ctx.encryptDetached({Out.data(), Data.size()}, Data);
  if (!Tag) {
    return WasiCryptoUnexpect(Tag);
  }

  // Gen tag
  Tag->assign(Out.begin(), Out.begin() + Data.size());

  return Out.size();
}

WasiCryptoExpect<SymmetricTag>
AesGcmSymmetricState::encryptDetachedUnchecked(Span<uint8_t> Out,
                                               Span<const uint8_t> Data) {
  auto Res = Ctx.encryptDetached(Out, Data);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SymmetricTag{Alg, std::move(*Res)};
}

WasiCryptoExpect<__wasi_size_t>
AesGcmSymmetricState::decryptUnchecked(Span<uint8_t> Out,
                                       Span<uint8_t const> Data) {

  Span<uint8_t const> RawTag{Data.data() + Out.size(),
                             Data.size() - Out.size()};

  return decryptDetachedUnchecked(Out, {Data.data(), Out.size()}, RawTag);
}

WasiCryptoExpect<__wasi_size_t> AesGcmSymmetricState::decryptDetachedUnchecked(
    Span<uint8_t> Out, Span<const uint8_t> Data, Span<uint8_t const> RawTag) {
  return Ctx.decryptDetached(Out, Data, RawTag);
}

AesGcmSymmetricState::AesGcmSymmetricState(SymmetricAlgorithm Alg,
                                           SymmetricOptions Options, AesGcm Ctx)
    : SymmetricState::Base(Alg), Options(Options), Ctx(std::move(Ctx)) {}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
