// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/symmetric/options.h"
#include "host/wasi_crypto/symmetric/state.h"
#include "host/wasi_crypto/util.h"
#include "openssl/sha.h"
#include "openssl/evp.h"

#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class Sha2SymmetricState : public SymmetricState {
public:
  static WasiCryptoExpect<std::unique_ptr<Sha2SymmetricState>>
  make(SymmetricAlgorithm Alg, std::shared_ptr<SymmetricKey> OptKey,
       std::shared_ptr<SymmetricOption> OptOptions);

  WasiCryptoExpect<void> absorb(Span<uint8_t const> Data) override;

  WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) override;

private:
  Sha2SymmetricState(SymmetricAlgorithm Alg,
                     std::shared_ptr<SymmetricOption> OptOptions, OpenSSlUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx);
  OpenSSlUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
