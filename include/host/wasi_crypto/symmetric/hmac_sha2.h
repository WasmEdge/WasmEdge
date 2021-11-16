// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/options.h"
#include "host/wasi_crypto/symmetric/state.h"
#include "host/wasi_crypto/util.h"
#include "openssl/evp.h"
#include "openssl/hmac.h"

#include <common/expected.h>
#include <cstdint>
#include <experimental/expected.hpp>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class HmacSha2SymmetricKey : public SymmetricKeyBase {
public:
  HmacSha2SymmetricKey(SymmetricAlgorithm Alg, Span<uint8_t const> Raw);

  ~HmacSha2SymmetricKey() override;

  WasiCryptoExpect<std::vector<uint8_t>> raw() override;

  SymmetricAlgorithm alg() override;

private:
  OpenSSlUniquePtr<EVP_PKEY, EVP_PKEY_free> PKey{EVP_PKEY_new()};
  SymmetricAlgorithm Alg;
  std::vector<uint8_t> Raw;
};

class HmacSha2KeyBuilder : public SymmetricKeyBuilder {
public:
  HmacSha2KeyBuilder(SymmetricAlgorithm Alg);

  WasiCryptoExpect<SymmetricKey> generate(std::optional<SymmetricOptions> OptOption) override;

  WasiCryptoExpect<SymmetricKey> import(Span<uint8_t const> Raw) override;

  WasiCryptoExpect<__wasi_size_t> keyLen() override;

private:
  SymmetricAlgorithm Alg;
};

class HmacSha2SymmetricState : public SymmetricStateBase {
public:
  static WasiCryptoExpect<std::unique_ptr<HmacSha2SymmetricState>>
  make(SymmetricAlgorithm Alg, std::optional<SymmetricKey> OptKey,
       std::optional<SymmetricOptions> OptOptions);

  WasiCryptoExpect<std::vector<uint8_t>>
  optionsGet(std::string_view Name) override;

  WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) override;

  WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override;

  WasiCryptoExpect<SymmetricTag> squeezeTag() override;

private:
  HmacSha2SymmetricState(SymmetricAlgorithm Alg,
                         std::optional<SymmetricOptions> OptOptions,
                         OpenSSlUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx);

  std::optional<SymmetricOptions> OptOptions;
  OpenSSlUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
