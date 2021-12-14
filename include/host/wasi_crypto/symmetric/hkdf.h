// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/state.h"
#include "host/wasi_crypto/wrapper/hkdf.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class HkdfSymmetricKey : public SymmetricKey::Base {
public:
  HkdfSymmetricKey(SymmetricAlgorithm Alg, Span<uint8_t const> Raw);

  Span<const uint8_t> asRef() { return Raw; }

  SymmetricAlgorithm alg() { return Alg; }

private:
  SymmetricAlgorithm Alg;
  std::vector<uint8_t> Raw;
};

class HkdfSymmetricKeyBuilder : public SymmetricKey::Builder {
public:
  HkdfSymmetricKeyBuilder(SymmetricAlgorithm Alg);

  WasiCryptoExpect<SymmetricKey>
  generate(std::optional<SymmetricOptions> OptOption) override;

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
class HkdfSymmetricState : public SymmetricState::Base {
public:
  static WasiCryptoExpect<std::unique_ptr<HkdfSymmetricState>>
  import(SymmetricAlgorithm Alg, std::optional<SymmetricKey> OptKey,
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
                     std::optional<SymmetricOptions> OptOptions, HkdfCtx Ctx);

  std::optional<SymmetricOptions> OptOptions;
  HkdfCtx Ctx;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
