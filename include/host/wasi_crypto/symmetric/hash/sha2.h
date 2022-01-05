// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/symmetric/hash/hash_state.h"
#include "host/wasi_crypto/symmetric/options.h"
#include "host/wasi_crypto/util.h"
#include "host/wasi_crypto/wrapper/sha2.h"

#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {
using EvpMdCtx = OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free>;

class Sha2State : public HashState {
public:
  Sha2State(SymmetricAlgorithm Alg, std::shared_ptr<Options> OptOptions,
            EVP_MD_CTX *Ctx)
      : OptOptions(OptOptions), Ctx(Ctx) {}

  WasiCryptoExpect<std::vector<uint8_t>>
  optionsGet(std::string_view Name) override;

  WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) override;

  WasiCryptoExpect<void> absorb(Span<uint8_t const> Data) override;

  WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) override;

private:
  std::shared_ptr<Options> OptOptions;
  EvpMdCtx Ctx;
};

WasiCryptoExpect<std::unique_ptr<Sha2State>>
import(SymmetricAlgorithm Alg, std::shared_ptr<Key> OptKey,
       std::shared_ptr<Options> OptOptions);

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
