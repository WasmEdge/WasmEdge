// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/wrapper/sha2.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<Sha2> Sha2::make(SymmetricAlgorithm Alg) {
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx{EVP_MD_CTX_new()};
  if (Ctx == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  EVP_MD const *Md;
  if (Alg == SymmetricAlgorithm::Sha256) {
    Md = EVP_sha256();
  } else if (Alg == SymmetricAlgorithm::Sha512) {
    Md = EVP_sha512();
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
  }

  if (1 != EVP_DigestInit_ex(Ctx.get(), Md, nullptr)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return Sha2(std::move(Ctx));
}

WasiCryptoExpect<void> Sha2::squeeze(Span<uint8_t> Out) {
  unsigned int ActualOutSize;
  auto CacheSize = EVP_MD_CTX_size(Ctx.get());
  uint8_t Cache[CacheSize];

  if (1 != EVP_DigestFinal_ex(Ctx.get(), Cache, &ActualOutSize)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  if (ActualOutSize > Out.size()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_LENGTH);
  }

  std::copy(Cache, Cache + ActualOutSize, Out.data());

  return {};
}

WasiCryptoExpect<void> Sha2::absorb(Span<const uint8_t> Data) {
  if (1 != EVP_DigestUpdate(Ctx.get(), Data.data(), Data.size())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  return {};
}

Sha2::Sha2(OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx)
    : Ctx(std::move(Ctx)) {}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
