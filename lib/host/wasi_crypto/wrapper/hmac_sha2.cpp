// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/wrapper/hmac_sha2.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<HmacSha2Ctx> HmacSha2Ctx::import(SymmetricAlgorithm Alg,
                                                  Span<uint8_t const> Raw) {
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> PKey{
      EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, nullptr, Raw.data(), Raw.size())};

  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx{EVP_MD_CTX_new()};


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

  if (1 != EVP_DigestSignInit(Ctx.get(), nullptr, Md, nullptr, PKey.get())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return HmacSha2Ctx{std::move(Ctx)};
}

WasiCryptoExpect<void> HmacSha2Ctx::absorb(Span<const uint8_t> Data) {
  if (1 != EVP_DigestSignUpdate(Ctx.get(), Data.data(), Data.size())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  return {};
}

WasiCryptoExpect<std::vector<uint8_t>> HmacSha2Ctx::squeezeTag() {
  size_t ActualOutSize;
  if (1 != EVP_DigestSignFinal(Ctx.get(), nullptr, &ActualOutSize)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  std::vector<uint8_t> Res;
  Res.reserve(ActualOutSize);
  Res.resize(ActualOutSize);

  if (1 != EVP_DigestSignFinal(Ctx.get(), Res.data(), &ActualOutSize)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return Res;
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
