// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/wrapper/hmac_sha2.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<HmacSha2> HmacSha2::make(SymmetricAlgorithm Alg) {

  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx{EVP_MD_CTX_new()};
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

  if (1 != EVP_DigestSignInit(Ctx.get(), nullptr, Md, nullptr, nullptr)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return HmacSha2{std::move(Ctx)};
}

WasiCryptoExpect<void> HmacSha2::setPKey(Span<uint8_t> Raw) {
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> PKey{
      EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, nullptr, Raw.data(), Raw.size())};
  if (PKey == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  if (1 !=
      EVP_DigestSignInit(Ctx.get(), nullptr, nullptr, nullptr, PKey.get())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  return {};
}

WasiCryptoExpect<void> HmacSha2::absorb(Span<const uint8_t> Data) {
  if (1 != EVP_DigestSignUpdate(Ctx.get(), Data.data(), Data.size())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  return {};
}

WasiCryptoExpect<Span<uint8_t>> HmacSha2::squeezeTag() {
  auto CacheSize = EVP_MD_CTX_size(Ctx.get());
  uint8_t Cache[CacheSize];
  size_t ActualOutSize;

  if (1 != EVP_DigestSignFinal(Ctx.get(), Cache, &ActualOutSize)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  Span<uint8_t> Res{*&Cache, ActualOutSize};
  return Res;
}

HmacSha2::HmacSha2(OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx)
    : Ctx(std::move(Ctx)) {}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
