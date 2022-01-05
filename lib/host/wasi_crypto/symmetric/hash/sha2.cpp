// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/hash/sha2.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

WasiCryptoExpect<std::vector<uint8_t>>
Sha2State::optionsGet(std::string_view Name) {
  if (!OptOptions) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }
  return OptOptions->inner()->locked(
      [&Name](auto &Inner) { return Inner.get(Name); });
}

WasiCryptoExpect<uint64_t>
Sha2State::optionsGetU64(std::string_view Name) {
  if (!OptOptions) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }
  return OptOptions->inner()->locked(
      [&Name](auto &Inner) { return Inner.getU64(Name); });
}

WasiCryptoExpect<void> Sha2State::absorb(Span<uint8_t const> Data) {
  opensslAssuming(EVP_DigestUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

WasiCryptoExpect<void> Sha2State::squeeze(Span<uint8_t> Out) {
  unsigned int ActualOutSize;
  auto CacheSize = EVP_MD_CTX_size(Ctx.get());
  std::vector<uint8_t> Cache;
  Cache.reserve(CacheSize);
  Cache.resize(CacheSize);

  opensslAssuming(EVP_DigestFinal_ex(Ctx.get(), Cache.data(), &ActualOutSize));
  if (ActualOutSize > Out.size()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_LENGTH);
  }

  std::copy(Cache.data(), Cache.data() + ActualOutSize, Out.data());

}


WasiCryptoExpect<std::unique_ptr<Sha2State>>
import(SymmetricAlgorithm Alg,
                  std::shared_ptr<Key> OptKey,
                  std::shared_ptr<Options> OptOptions) {
  if (OptKey) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_KEY_NOT_SUPPORTED);
  }

  EVP_MD_CTX* Ctx = EVP_MD_CTX_new();
  opensslAssuming(Ctx);

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

  opensslAssuming(EVP_DigestInit(Ctx, Md));

  return std::unique_ptr<Sha2State>{
      new Sha2State(Alg, OptOptions, Ctx)};
}
} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
