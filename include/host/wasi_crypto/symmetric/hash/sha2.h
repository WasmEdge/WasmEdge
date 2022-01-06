// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/symmetric/hash/hash_state.h"
#include "host/wasi_crypto/symmetric/options.h"
#include "host/wasi_crypto/util.h"

#include <openssl/evp.h>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

class Sha2State : public HashState {
  using EvpMdCtx = OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free>;

public:
  Sha2State(std::shared_ptr<Option> OptOption, EVP_MD_CTX *Ctx)
      : OptOption(OptOption), Ctx(Ctx) {}

  static WasiCryptoExpect<std::unique_ptr<Sha2State>>
  open(SymmetricAlgorithm Alg, std::shared_ptr<Key> OptKey,
       std::shared_ptr<Option> OptOption);

  WasiCryptoExpect<std::vector<uint8_t>>
  optionsGet(std::string_view Name) override;

  WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) override;

  WasiCryptoExpect<void> absorb(Span<uint8_t const> Data) override;

  WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) override;

private:
  std::shared_ptr<Option> OptOption;
  EvpMdCtx Ctx;
};

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
