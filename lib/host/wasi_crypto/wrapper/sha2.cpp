// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/wrapper/sha2.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<Sha2Ctx> Sha2Ctx::make(SymmetricAlgorithm Alg) {
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx{EVP_MD_CTX_new()};

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

  return Sha2Ctx(std::move(Ctx));
}

WasiCryptoExpect<void> Sha2Ctx::squeeze(Span<uint8_t> Out) {
  unsigned int ActualOutSize;
  auto CacheSize = EVP_MD_CTX_size(Ctx.get());
  std::vector<uint8_t> Cache;
  Cache.reserve(CacheSize);
  Cache.resize(CacheSize);

  if (1 != EVP_DigestFinal_ex(Ctx.get(), Cache.data(), &ActualOutSize)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  if (ActualOutSize > Out.size()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_LENGTH);
  }

  std::copy(Cache.data(), Cache.data() + ActualOutSize, Out.data());

  return {};
}

WasiCryptoExpect<void> Sha2Ctx::absorb(Span<const uint8_t> Data) {
  if (1 != EVP_DigestUpdate(Ctx.get(), Data.data(), Data.size())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  return {};
}

Sha2Ctx::Sha2Ctx(OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx)
    : Ctx(std::move(Ctx)) {}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
