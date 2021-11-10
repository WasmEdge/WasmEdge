// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/sha2.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<std::unique_ptr<Sha2SymmetricState>>
Sha2SymmetricState::make(SymmetricAlgorithm Alg,
                         std::shared_ptr<SymmetricKey> OptKey,
                         std::shared_ptr<SymmetricOption> OptOptions) {
  if (OptKey != nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_KEY_NOT_SUPPORTED);
  }

  OpenSSlUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx{EVP_MD_CTX_new()};
  EVP_MD const *Md;
  switch (Alg) {
  case SymmetricAlgorithm::Sha256:
    Md = EVP_sha256();
    break;
  case SymmetricAlgorithm::Sha512:
    Md = EVP_sha512();
    break;
  case SymmetricAlgorithm::Sha512_256:
    Md = EVP_sha512_256();
    break;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
  }
  EVP_DigestInit_ex(Ctx.get(), Md, nullptr);

  return std::unique_ptr<Sha2SymmetricState>{
      new Sha2SymmetricState(Alg, OptOptions, std::move(Ctx))};
}

WasiCryptoExpect<void> Sha2SymmetricState::absorb(Span<uint8_t const> Data) {
  EVP_DigestUpdate(Ctx.get(), Data.data(), Data.size());
  return {};
}

WasiCryptoExpect<void> Sha2SymmetricState::squeeze(Span<uint8_t> Out) {
  unsigned int ActualOutSize;
  EVP_DigestFinal_ex(Ctx.get(), Out.data(), &ActualOutSize);

  return SymmetricState::squeeze(Out);
}

Sha2SymmetricState::Sha2SymmetricState(
    SymmetricAlgorithm Alg, std::shared_ptr<SymmetricOption> OptOptions,
    OpenSSlUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx)
    : SymmetricState(Alg, std::move(OptOptions)), Ctx(std::move(Ctx)) {}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
