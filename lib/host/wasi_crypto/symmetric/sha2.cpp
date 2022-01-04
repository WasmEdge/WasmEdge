// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/sha2.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<std::unique_ptr<Sha2SymmetricState>>
Sha2SymmetricState::import(SymmetricAlgorithm Alg,
                           std::optional<SymmetricKey> OptKey,
                           std::optional<SymmetricOptions> OptOptions) {
  if (OptKey) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_KEY_NOT_SUPPORTED);
  }

  auto Res = Sha2Ctx::import(Alg);
  if (!Res) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return std::unique_ptr<Sha2SymmetricState>{
      new Sha2SymmetricState(Alg, OptOptions, std::move(*Res))};
}

WasiCryptoExpect<std::vector<uint8_t>>
Sha2SymmetricState::optionsGet(std::string_view Name) {
  if (!OptOptions) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }
  return OptOptions->inner()->locked(
      [&Name](auto &Inner) { return Inner.get(Name); });
}

WasiCryptoExpect<uint64_t>
Sha2SymmetricState::optionsGetU64(std::string_view Name) {
  if (!OptOptions) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  }
  return OptOptions->inner()->locked(
      [&Name](auto &Inner) { return Inner.getU64(Name); });
}

WasiCryptoExpect<void> Sha2SymmetricState::absorb(Span<uint8_t const> Data) {
  return Ctx.absorb(Data);
}

WasiCryptoExpect<void> Sha2SymmetricState::squeeze(Span<uint8_t> Out) {
  return Ctx.squeeze(Out);
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
