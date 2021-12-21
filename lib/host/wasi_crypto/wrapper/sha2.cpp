// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/wrapper/sha2.h"
#include "common/errcode.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<Sha2Ctx> Sha2Ctx::import(SymmetricAlgorithm Alg) {
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx{EVP_MD_CTX_new()};
  assuming(Ctx);

  EVP_MD const *Md;
  switch (Alg) {
  case SymmetricAlgorithm::Sha256:
    Md = EVP_sha256();
    break;
  case SymmetricAlgorithm::Sha512:
    Md = EVP_sha512();
    break;
  default:
    __builtin_unreachable();
  }

  assuming(EVP_DigestInit_ex(Ctx.get(), Md, nullptr));

  return Sha2Ctx(std::move(Ctx));
}

WasiCryptoExpect<void> Sha2Ctx::squeeze(Span<uint8_t> Out) {
  unsigned int ActualOutSize;
  auto CacheSize = EVP_MD_CTX_size(Ctx.get());
  std::vector<uint8_t> Cache;
  Cache.reserve(CacheSize);
  Cache.resize(CacheSize);

  assuming(EVP_DigestFinal_ex(Ctx.get(), Cache.data(), &ActualOutSize));
  if (ActualOutSize > Out.size()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_LENGTH);
  }

  std::copy(Cache.data(), Cache.data() + ActualOutSize, Out.data());

  return {};
}

WasiCryptoExpect<void> Sha2Ctx::absorb(Span<const uint8_t> Data) {
  assuming(EVP_DigestUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
