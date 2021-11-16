// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/symmetric/options.h"
#include "host/wasi_crypto/symmetric/state.h"
#include "host/wasi_crypto/util.h"
#include "openssl/evp.h"
#include "openssl/sha.h"

#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class Sha2SymmetricState : public SymmetricStateBase {
public:
  static WasiCryptoExpect<std::unique_ptr<Sha2SymmetricState>>
  make(SymmetricAlgorithm Alg, std::optional<SymmetricKey> OptKey,
       std::optional<SymmetricOptions> OptOptions);

  WasiCryptoExpect<std::vector<uint8_t>>
  optionsGet(std::string_view Name) override;

  WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) override;

  WasiCryptoExpect<void> absorb(Span<uint8_t const> Data) override;

  WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) override;

private:
  Sha2SymmetricState(SymmetricAlgorithm Alg,
                     std::optional<SymmetricOptions> OptOptions,
                     OpenSSlUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx);

  std::optional<SymmetricOptions> OptOptions;
  OpenSSlUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
