// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/symmetric/aeads/aeads_state.h"
#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/tag.h"

#include "host/wasi_crypto/evpwrapper.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

template <uint32_t KeyBit> class AesGcm {
public:
  class KeyBuilder final : public Key::Builder {
  public:
    using Builder::Builder;

    WasiCryptoExpect<std::unique_ptr<Key>>
    generate(std::shared_ptr<Options> OptOption) override;

    WasiCryptoExpect<std::unique_ptr<Key>>
    import(Span<uint8_t const> Raw) override;
  };

  // Nonce = IV,
  class State final : public AEADsState {
  public:
    // 96 bit
    inline static constexpr size_t NonceSize = 12;

    inline static constexpr size_t TagLen = 16;

    State(EvpCipherCtxPtr Ctx, std::shared_ptr<Options> OptOption)
        : Ctx(std::move(Ctx)), OptOption(OptOption) {}

    /// There are four inputs for authenticated encryption:
    /// @param[in] OptKey The secret key for encrypt
    /// @param[in] OptOption `Must` contain an Nonce(Initialization vector).
    static WasiCryptoExpect<std::unique_ptr<State>>
    open(std::shared_ptr<Key> OptKey, std::shared_ptr<Options> OptOption);

    WasiCryptoExpect<std::vector<uint8_t>>
    optionsGet(std::string_view Name) override;

    WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) override;

    /// @param[in] Data additional authentication data(AAD)
    WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override;

    WasiCryptoExpect<size_t> maxTagLen() override { return TagLen; }

  protected:
    /// @param[out] Out The encrypted text.
    /// @param[in] Data The plain text.
    /// @return Tag.
    WasiCryptoExpect<Tag>
    encryptDetachedUnchecked(Span<uint8_t> Out,
                             Span<const uint8_t> Data) override;

    /// @param[out] Out The plain text.
    /// @param[in] Data The encrypted text.
    /// @param[in] RawTag Tag.
    WasiCryptoExpect<size_t>
    decryptDetachedUnchecked(Span<uint8_t> Out, Span<const uint8_t> Data,
                             Span<uint8_t const> RawTag) override;

  private:
    enum Mode { Unchanged = -1, Decrypt = 0, Encrypt = 1 };
    EvpCipherCtxPtr Ctx;
    std::shared_ptr<Options> OptOption;
  };

private:
  constexpr static const EVP_CIPHER *getCipher() {
    switch (KeyBit) {
    case 128:
      return EVP_aes_128_gcm();
    case 256:
      return EVP_aes_256_gcm();
    default:
      assumingUnreachable();
    }
  }
};

using AesGcm128 = AesGcm<128>;
using AesGcm256 = AesGcm<256>;

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
