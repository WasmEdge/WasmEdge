// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/symmetric/extract_and_expand/eae_state.h"

#include "host/wasi_crypto/evpwrapper.h"
#include "openssl/kdf.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

template <uint32_t Sha, uint32_t Mode> class Hkdf {
public:
  class KeyBuilder final : public Key::Builder {
  public:
    using Builder::Builder;

    WasiCryptoExpect<std::unique_ptr<Key>>
    generate(std::shared_ptr<Options> OptOption) override;

    WasiCryptoExpect<std::unique_ptr<Key>>
    import(Span<uint8_t const> Raw) override;

    size_t keyLen() override;
  };

  /// Two case:
  /// Extract:
  ///
  /// Expand:
  ///
  class State final : public ExtractAndExpandState {
  public:
    State(std::shared_ptr<Options> OptOption, EVP_PKEY_CTX *Ctx)
        : OptOption(OptOption), Ctx(Ctx) {}

    ~State() override;

    static WasiCryptoExpect<std::unique_ptr<State>>
    open(std::shared_ptr<Key> OptKey, std::shared_ptr<Options> OptOption);

    /// absorbs the salt of the key(Extract)/info(Expand) information.
    WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override;

    /// Extract:
    /// returns the PRK, whose algorithm type is set to the EXPAND counterpart
    /// of the EXTRACT operation
    WasiCryptoExpect<std::unique_ptr<Key>>
    squeezeKey(SymmetricAlgorithm Alg) override;

    // Expand
    WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) override;

    WasiCryptoExpect<std::vector<uint8_t>>
    optionsGet(std::string_view Name) override;

    WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) override;

  private:
    std::shared_mutex Mutex;
    std::vector<uint8_t> Cache;
    std::shared_ptr<Options> OptOption;
    EvpPkeyCtxPtr Ctx;
  };
};

using Hkdf256Extract = Hkdf<256, EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY>;
using Hkdf512Extract = Hkdf<512, EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY>;
using Hkdf256Expand = Hkdf<256, EVP_PKEY_HKDEF_MODE_EXPAND_ONLY>;
using Hkdf512Expand = Hkdf<512, EVP_PKEY_HKDEF_MODE_EXPAND_ONLY>;

} // namespace Symmetric

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
