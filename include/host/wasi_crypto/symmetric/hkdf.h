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

class HkdfSymmetricKey : public SymmetricKey {
public:
  HkdfSymmetricKey(SymmetricAlgorithm Alg, Span<uint8_t const> Raw);
  WasiCryptoExpect<Span<uint8_t>> raw() override;
  SymmetricAlgorithm alg() override;

private:
  SymmetricAlgorithm Alg;
  std::vector<uint8_t> Raw;
};

class HkdfSymmetricKeyBuilder : public SymmetricKeyBuilder {
public:
  HkdfSymmetricKeyBuilder(SymmetricAlgorithm Alg);

  WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
  generate(std::shared_ptr<SymmetricOption> Option) override;

  WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
  import(Span<uint8_t const> Raw) override;

  WasiCryptoExpect<__wasi_size_t> keyLen() override;

private:
  SymmetricAlgorithm Alg;
};

/// Two case:
/// Extract:
///
/// Expand:
///
class HkdfSymmetricState : public SymmetricState {
public:
  static WasiCryptoExpect<std::unique_ptr<HkdfSymmetricState>>
  make(SymmetricAlgorithm Alg, std::shared_ptr<SymmetricKey> OptKey,
       std::shared_ptr<SymmetricOption> OptOptions);

  /// absorbs the salt of the key(Extract)/info(Expand) information.
  WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override;

  /// Extract:
  /// returns the PRK, whose algorithm type is set to the EXPAND counterpart of the EXTRACT operation
  WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
  squeezeKey(SymmetricAlgorithm AlgStr) override;

  // Expand
  WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) override;

private:
  HkdfSymmetricState(SymmetricAlgorithm Algorithm,
                     std::shared_ptr<SymmetricOption> OptOptions, OpenSSlUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx);

  OpenSSlUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
