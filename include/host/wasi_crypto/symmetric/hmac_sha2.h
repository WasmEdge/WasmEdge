// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/options.h"
#include "host/wasi_crypto/symmetric/state.h"
#include "host/wasi_crypto/util.h"
#include "host/wasi_crypto/wrapper/hmac_sha2.h"
#include "openssl/evp.h"
#include "openssl/hmac.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class HmacSha2SymmetricKey : public SymmetricKey::Base {
public:
  HmacSha2SymmetricKey(SymmetricAlgorithm Alg, Span<uint8_t const> Raw);

  ~HmacSha2SymmetricKey() override;

  Span<const uint8_t> asRef() { return Raw; }

  SymmetricAlgorithm alg() { return Alg; }

private:
  SymmetricAlgorithm Alg;
  std::vector<uint8_t> Raw;
};

class HmacSha2KeyBuilder : public SymmetricKey::Builder {
public:
  HmacSha2KeyBuilder(SymmetricAlgorithm Alg);

  WasiCryptoExpect<SymmetricKey>
  generate(std::optional<SymmetricOptions> OptOption) override;

  WasiCryptoExpect<SymmetricKey> import(Span<uint8_t const> Raw) override;

  WasiCryptoExpect<__wasi_size_t> keyLen() override;

private:
  SymmetricAlgorithm Alg;
};

class HmacSha2SymmetricState : public SymmetricState::Base {
public:
  static WasiCryptoExpect<std::unique_ptr<HmacSha2SymmetricState>>
  import(SymmetricAlgorithm Alg, std::optional<SymmetricKey> OptKey,
         std::optional<SymmetricOptions> OptOptions);

  WasiCryptoExpect<std::vector<uint8_t>>
  optionsGet(std::string_view Name) override;

  WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) override;

  WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override;

  WasiCryptoExpect<SymmetricTag> squeezeTag() override;

private:
  HmacSha2SymmetricState(SymmetricAlgorithm Alg,
                         std::optional<SymmetricOptions> OptOptions,
                         HmacSha2Ctx Ctx);

  std::optional<SymmetricOptions> OptOptions;
  HmacSha2Ctx Ctx;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
