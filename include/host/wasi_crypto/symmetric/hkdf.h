// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/expected.h"
#include "experimental/expected.hpp"
#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/state.h"

#include "openssl/evp.h"
#include "openssl/kdf.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class HkdfSymmetricKey : public SymmetricKeyBase {
public:
  HkdfSymmetricKey(SymmetricAlgorithm Alg, Span<uint8_t const> Raw);
  WasiCryptoExpect<std::vector<uint8_t>> raw() override;
  SymmetricAlgorithm alg() override;

private:
  SymmetricAlgorithm Alg;
  std::vector<uint8_t> Raw;
};

class HkdfSymmetricKeyBuilder : public SymmetricKeyBuilder {
public:
  HkdfSymmetricKeyBuilder(SymmetricAlgorithm Alg);

  WasiCryptoExpect<SymmetricKey> generate(std::optional<SymmetricOptions> OptOption) override;

  WasiCryptoExpect<SymmetricKey> import(Span<uint8_t const> Raw) override;

  WasiCryptoExpect<__wasi_size_t> keyLen() override;

private:
  SymmetricAlgorithm Alg;
};

/// Two case:
/// Extract:
///
/// Expand:
///
class HkdfSymmetricState : public SymmetricStateBase {
public:
  static WasiCryptoExpect<std::unique_ptr<HkdfSymmetricState>>
  make(SymmetricAlgorithm Alg, std::optional<SymmetricKey> OptKey,
       std::optional<SymmetricOptions> OptOptions);

  /// absorbs the salt of the key(Extract)/info(Expand) information.
  WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override;

  /// Extract:
  /// returns the PRK, whose algorithm type is set to the EXPAND counterpart of
  /// the EXTRACT operation
  WasiCryptoExpect<SymmetricKey> squeezeKey(SymmetricAlgorithm Alg) override;

  // Expand
  WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) override;

  WasiCryptoExpect<std::vector<uint8_t>>
  optionsGet(std::string_view Name) override;

  WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) override;

private:
  HkdfSymmetricState(SymmetricAlgorithm Algorithm,
                     std::optional<SymmetricOptions> OptOptions,
                     OpenSSlUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx);

  std::optional<SymmetricOptions> OptOptions;
  OpenSSlUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
