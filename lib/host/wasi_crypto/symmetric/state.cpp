// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/state.h"
#include "host/wasi_crypto/symmetric/aes_gcm.h"
#include "host/wasi_crypto/symmetric/charcha_poly.h"
#include "host/wasi_crypto/symmetric/hkdf.h"
#include "host/wasi_crypto/symmetric/hmac_sha2.h"
#include "host/wasi_crypto/symmetric/sha2.h"
#include "host/wasi_crypto/symmetric/xoodyak.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

namespace {
constexpr bool checkAdd(Span<const uint8_t> Data, __wasi_size_t TagSize) {
  return (SIZE_MAX - TagSize) < Data.size();
}

constexpr bool checkSub(Span<const uint8_t> Data, __wasi_size_t TagSize) {
  return TagSize > Data.size();
}

} // namespace

WasiCryptoExpect<std::vector<uint8_t>>
SymmetricState::optionsGet(std::string_view Name) {
  if (!Options) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }

  return Options->get(Name);
}

WasiCryptoExpect<uint64_t>
SymmetricState::optionsGetU64(std::string_view Name) {
  if (!Options) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }

  return Options->getU64(Name);
}

WasiCryptoExpect<std::shared_ptr<SymmetricOption>>
SymmetricState::options(std::string_view) {
  return Options;
}

WasiCryptoExpect<void> SymmetricState::absorb(Span<uint8_t const>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<void> SymmetricState::squeeze(Span<uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
SymmetricState::squeezeKey(std::string_view) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<SymmetricTag> SymmetricState::squeezeTag() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<__wasi_size_t> SymmetricState::maxTagLen() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<__wasi_size_t>
SymmetricState::encryptUnchecked(Span<uint8_t>, Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<SymmetricTag>
SymmetricState::encryptDetachedUnchecked(Span<uint8_t>, Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<__wasi_size_t>
SymmetricState::decryptUnchecked(Span<uint8_t>, Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<__wasi_size_t>
SymmetricState::decryptDetachedUnchecked(Span<uint8_t>, Span<const uint8_t>,
                                         Span<uint8_t const>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<__wasi_size_t>
SymmetricState::encrypt(Span<uint8_t> Out, Span<const uint8_t> Data) {
  auto TagSize = maxTagLen();
  if (!TagSize) {
    return WasiCryptoUnexpect(TagSize);
  }

  if (checkAdd(Data, *TagSize)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OVERFLOW);
  }
  if (Out.size() != Data.size() + *TagSize) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_LENGTH);
  }

  return encryptUnchecked(Out, Data);
}

WasiCryptoExpect<SymmetricTag>
SymmetricState::encryptDetached(Span<uint8_t> Out, Span<const uint8_t> Data) {
  if (Out.size() != Data.size())
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_LENGTH);
  return encryptDetachedUnchecked(Out, Data);
}

WasiCryptoExpect<__wasi_size_t>
SymmetricState::decrypt(Span<uint8_t> Out, Span<const uint8_t> Data) {
  if (auto TagSize = maxTagLen(); !TagSize) {
    return WasiCryptoUnexpect(TagSize);
  } else {
    if (checkSub(Data, *TagSize))
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OVERFLOW);
    if (Out.size() != Data.size() - *TagSize)
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OVERFLOW);
  }

  auto Result = decryptUnchecked(Out, Data);
  if (!Result) {
    std::fill(Out.begin(), Out.end(), 0);
    return WasiCryptoUnexpect(Result);
  }

  return *Result;
}

WasiCryptoExpect<__wasi_size_t>
SymmetricState::decryptDetached(Span<uint8_t> Out, Span<const uint8_t> Data,
                                Span<uint8_t> RawTag) {

  if (Out.size() != Data.size()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_LENGTH);
  }

  auto Result = decryptDetachedUnchecked(Out, Data, RawTag);
  if (!Result) {
    std::fill(Out.begin(), Out.end(), 0);
    return WasiCryptoUnexpect(Result);
  }

  return *Result;
}

WasiCryptoExpect<void> SymmetricState::ratchet() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
}

WasiCryptoExpect<std::unique_ptr<SymmetricState>>
SymmetricState::make(std::string_view Alg,
                     std::shared_ptr<SymmetricKey> KeyOptional,
                     std::shared_ptr<SymmetricOption> OptionsOptional) {
  auto EnumAlg = fromConstantString(Alg);
  if (!EnumAlg) {
    return WasiCryptoUnexpect(EnumAlg);
  }

  if (KeyOptional != nullptr && (KeyOptional->alg() != EnumAlg)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }
  switch (*EnumAlg) {
  case SymmetricAlgorithm::HmacSha256:
  case SymmetricAlgorithm::HmacSha512:
    return HmacSha2SymmetricState::make(*EnumAlg, std::move(KeyOptional),
                                        std::move(OptionsOptional));
  case SymmetricAlgorithm::HkdfSha256Extract:
  case SymmetricAlgorithm::HkdfSha512Extract:
  case SymmetricAlgorithm::HkdfSha256Expand:
  case SymmetricAlgorithm::HkdfSha512Expand:
    return HkdfSymmetricState::make(*EnumAlg, KeyOptional, OptionsOptional);
  case SymmetricAlgorithm::Sha256:
  case SymmetricAlgorithm::Sha512:
  case SymmetricAlgorithm::Sha512_256:
    return Sha2SymmetricState::make(*EnumAlg, nullptr,
                                    std::move(OptionsOptional));
  case SymmetricAlgorithm::Aes128Gcm:
  case SymmetricAlgorithm::Aes256Gcm:
    return AesGcmSymmetricState::make(*EnumAlg, std::move(KeyOptional),
                                      std::move(OptionsOptional));
  case SymmetricAlgorithm::ChaCha20Poly1305:
  case SymmetricAlgorithm::XChaCha20Poly1305:
    return ChaChaPolySymmetricState::make(*EnumAlg, std::move(KeyOptional),
                                          std::move(OptionsOptional));
  case SymmetricAlgorithm::Xoodyak128:
  case SymmetricAlgorithm::Xoodyak160:
    return XoodyakSymmetricState::make(*EnumAlg, std::move(KeyOptional),
                                       std::move(OptionsOptional));
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
  }
}

SymmetricState::SymmetricState(SymmetricAlgorithm Alg,
                               std::shared_ptr<SymmetricOption> Optional)
    : Alg(Alg), Options(Optional) {}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
