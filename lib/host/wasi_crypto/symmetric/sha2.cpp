// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/sha2.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<std::unique_ptr<Sha2SymmetricState>>
Sha2SymmetricState::make(SymmetricAlgorithm Alg,
                         std::optional<SymmetricKey> OptKey,
                         std::optional<SymmetricOptions> OptOptions) {
  if (OptKey) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_KEY_NOT_SUPPORTED);
  }

  OpenSSlUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx{EVP_MD_CTX_new()};
  if (Ctx == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

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

  if (1 != EVP_DigestInit_ex(Ctx.get(), Md, nullptr)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return std::unique_ptr<Sha2SymmetricState>{
      new Sha2SymmetricState(Alg, OptOptions, std::move(Ctx))};
}

WasiCryptoExpect<std::vector<uint8_t>>
Sha2SymmetricState::optionsGet(std::string_view Name) {
  if (!OptOptions) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }
  return OptOptions->get(Name);
}

WasiCryptoExpect<uint64_t>
Sha2SymmetricState::optionsGetU64(std::string_view Name) {
  if (!OptOptions) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }
  return OptOptions->getU64(Name);
}

WasiCryptoExpect<void> Sha2SymmetricState::absorb(Span<uint8_t const> Data) {
  EVP_DigestUpdate(Ctx.get(), Data.data(), Data.size());
  return {};
}

WasiCryptoExpect<void> Sha2SymmetricState::squeeze(Span<uint8_t> Out) {
  unsigned int ActualOutSize;
  auto CacheSize = EVP_MD_CTX_size(Ctx.get());
  uint8_t Cache[CacheSize];

  EVP_DigestFinal_ex(Ctx.get(), Cache, &ActualOutSize);
  if (ActualOutSize > Out.size()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_LENGTH);
  }

  std::copy(Cache, Cache + ActualOutSize, Out.data());

  return {};
}

Sha2SymmetricState::Sha2SymmetricState(
    SymmetricAlgorithm Alg, std::optional<SymmetricOptions> OptOptions,
    OpenSSlUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx)
    : SymmetricStateBase(Alg), OptOptions(OptOptions), Ctx(std::move(Ctx)) {}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
