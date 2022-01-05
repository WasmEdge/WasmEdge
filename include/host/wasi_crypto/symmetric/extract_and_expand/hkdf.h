// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/symmetric/extract_and_expand/eae_state.h"
#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/wrapper/hkdf.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {
using EvpPkeyCtxPtr = OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free>;

class HkdfKeyBuilder : public Key::Builder {
public:
  HkdfKeyBuilder(SymmetricAlgorithm Alg);

  WasiCryptoExpect<Key> generate(std::optional<Options> OptOption) override;

  WasiCryptoExpect<Key> import(Span<uint8_t const> Raw) override;

  WasiCryptoExpect<__wasi_size_t> keyLen() override;

private:
  SymmetricAlgorithm Alg;
};

/// Two case:
/// Extract:
///
/// Expand:
///
class HkdfState : public ExtractAndExpandState {
public:
  HkdfState(SymmetricAlgorithm Algorithm,
                     std::shared_ptr<Options> OptOptions, EVP_PKEY_CTX *Ctx)
      : SymmetricState::Base(Algorithm), OptOptions(OptOptions), Ctx(Ctx) {}

  static WasiCryptoExpect<std::unique_ptr<HkdfState>>
  import(SymmetricAlgorithm Alg, std::shared_ptr<Key> OptKey,
         std::shared_ptr<Options> OptOptions);

  /// absorbs the salt of the key(Extract)/info(Expand) information.
  WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override;

  /// Extract:
  /// returns the PRK, whose algorithm type is set to the EXPAND counterpart of
  /// the EXTRACT operation
  WasiCryptoExpect<Key> squeezeKey(SymmetricAlgorithm Alg) override;

  // Expand
  WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) override;

  WasiCryptoExpect<std::vector<uint8_t>>
  optionsGet(std::string_view Name) override;

  WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) override;

private:
  SymmetricAlgorithm Alg;
  std::shared_ptr<Options> OptOptions;
  EvpPkeyCtxPtr Ctx;
};



} // namespace Symmetric

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
