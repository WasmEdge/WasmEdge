// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/wrapper/hmac_sha2.h"
#include "common/errcode.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<HmacSha2Ctx> HmacSha2Ctx::import(SymmetricAlgorithm Alg,
                                                  Span<uint8_t const> Raw) {
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> PKey{
      EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, nullptr, Raw.data(), Raw.size())};
  assuming(PKey);

  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx{EVP_MD_CTX_new()};
  assuming(Ctx);

  EVP_MD const *Md;
  switch (Alg) {
  case SymmetricAlgorithm::HmacSha256:
    Md = EVP_sha256();
    break;
  case SymmetricAlgorithm::HmacSha512:
    Md = EVP_sha512();
    break;
  default:
    __builtin_unreachable();
  }

  assuming(EVP_DigestSignInit(Ctx.get(), nullptr, Md, nullptr, PKey.get()));

  return HmacSha2Ctx{std::move(Ctx)};
}

WasiCryptoExpect<void> HmacSha2Ctx::absorb(Span<const uint8_t> Data) {
  assuming(EVP_DigestSignUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

WasiCryptoExpect<std::vector<uint8_t>> HmacSha2Ctx::squeezeTag() {
  size_t ActualOutSize;
  assuming(EVP_DigestSignFinal(Ctx.get(), nullptr, &ActualOutSize));

  std::vector<uint8_t> Res;
  Res.reserve(ActualOutSize);
  Res.resize(ActualOutSize);

  assuming(EVP_DigestSignFinal(Ctx.get(), Res.data(), &ActualOutSize));

  return Res;
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
