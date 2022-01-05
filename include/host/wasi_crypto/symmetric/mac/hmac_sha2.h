// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/mac/mac_state.h"
#include "host/wasi_crypto/symmetric/options.h"
#include "host/wasi_crypto/util.h"

#include "openssl/evp.h"
#include "openssl/hmac.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {
using EvpMdCtxPtr = OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free>;

class HmacSha2KeyBuilder : public Key::Builder {
public:
  HmacSha2KeyBuilder(SymmetricAlgorithm Alg);

  WasiCryptoExpect<Key> generate(std::optional<Options> OptOption) override;

  WasiCryptoExpect<Key> import(Span<uint8_t const> Raw) override;

  WasiCryptoExpect<__wasi_size_t> keyLen() override;

private:
  SymmetricAlgorithm Alg;
};

class HmacSha2State : public MACState {
public:
  HmacSha2State(SymmetricAlgorithm Alg,
                std::shared_ptr<Options> OptOptions, EVP_MD_CTX *Ctx)
      : SymmetricState::Base(Alg), OptOptions(OptOptions), Ctx(Ctx) {}

  static WasiCryptoExpect<std::unique_ptr<HmacSha2State>>
  import(SymmetricAlgorithm Alg, std::shared_ptr<Key> OptKey,
         std::shared_ptr<Options> OptOptions);

  WasiCryptoExpect<std::vector<uint8_t>>
  optionsGet(std::string_view Name) override;

  WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) override;

  WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override;

  WasiCryptoExpect<Tag> squeezeTag() override;

  std::shared_ptr<Options> OptOptions;
  EvpMdCtxPtr Ctx;
};


} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
