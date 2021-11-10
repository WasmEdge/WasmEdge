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

class HmacSha2SymmetricKey : public SymmetricKey {
public:
  HmacSha2SymmetricKey(SymmetricAlgorithm Alg, Span<uint8_t const> Raw);
  WasiCryptoExpect<Span<uint8_t>> raw() override;
  SymmetricAlgorithm alg() override;

private:
  SymmetricAlgorithm Alg;
  std::vector<uint8_t> Raw;
};

class HmacSha2KeyBuilder : public SymmetricKeyBuilder {
public:
  HmacSha2KeyBuilder(SymmetricAlgorithm Alg);

  WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
  generate(std::shared_ptr<SymmetricOption> Option) override;

  WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
  import(Span<uint8_t const> Raw) override;

  WasiCryptoExpect<__wasi_size_t> keyLen() override;

private:
  SymmetricAlgorithm Alg;
};

// TODO:  Is deprecated: Since OpenSSL 3.0, use EVP_ to instead.
class HmacSha2SymmetricState : public SymmetricState {
public:
  static WasiCryptoExpect<std::unique_ptr<HmacSha2SymmetricState>>
  make(SymmetricAlgorithm Alg, std::shared_ptr<SymmetricKey> OptKey,
       std::shared_ptr<SymmetricOption> OptOptions);

  WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override;

  WasiCryptoExpect<SymmetricTag> squeezeTag() override;

private:
  HmacSha2SymmetricState(SymmetricAlgorithm Alg, Span<uint8_t> Raw,
                         std::shared_ptr<SymmetricOption> OptOptions);

  OpenSSlUniquePtr<HMAC_CTX, HMAC_CTX_free> Ctx{HMAC_CTX_new()};
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
