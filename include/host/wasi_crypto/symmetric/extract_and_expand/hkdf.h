// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <openssl/evp.h>
#include "host/wasi_crypto/symmetric/extract_and_expand/eae_state.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {
using EvpPkeyCtxPtr = OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free>;

class HkdfKeyBuilder : public Key::Builder {
public:
  using Builder::Builder;

  WasiCryptoExpect<std::unique_ptr<Key>> generate(std::shared_ptr<Option> OptOption) override;

  WasiCryptoExpect<std::unique_ptr<Key>> import(Span<uint8_t const> Raw) override;

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
  HkdfState(
                     std::shared_ptr<Option> OptOption, EVP_PKEY_CTX *Ctx)
      : OptOption(OptOption), Ctx(Ctx) {}

  static WasiCryptoExpect<std::unique_ptr<HkdfState>>
  open(SymmetricAlgorithm Alg, std::shared_ptr<Key> OptKey,
         std::shared_ptr<Option> OptOption);

  /// absorbs the salt of the key(Extract)/info(Expand) information.
  WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override;

  /// Extract:
  /// returns the PRK, whose algorithm type is set to the EXPAND counterpart of
  /// the EXTRACT operation
  WasiCryptoExpect<std::unique_ptr<Key>> squeezeKey(SymmetricAlgorithm Alg) override;

  // Expand
  WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) override;

  WasiCryptoExpect<std::vector<uint8_t>>
  optionsGet(std::string_view Name) override;

  WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) override;

private:
  std::shared_ptr<Option> OptOption;
  std::optional<std::vector<uint8_t>> Cache;
  EvpPkeyCtxPtr Ctx;
};



} // namespace Symmetric

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
