// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/state.h"
#include "host/wasi_crypto/symmetric/tag.h"
#include "host/wasi_crypto/wrapper/aes_gcm.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class AesGcmSymmetricKey : public SymmetricKey::Base {
public:
  AesGcmSymmetricKey(SymmetricAlgorithm Alg, Span<uint8_t const> Raw);

  Span<const uint8_t> asRef() override { return Raw; }

  SymmetricAlgorithm alg() override { return Alg;}

private:
  SymmetricAlgorithm Alg;
  std::vector<uint8_t> Raw;
};

class AesGcmSymmetricKeyBuilder : public SymmetricKey::Builder {
public:
  AesGcmSymmetricKeyBuilder(SymmetricAlgorithm Alg);

  WasiCryptoExpect<SymmetricKey>
  generate(std::optional<SymmetricOptions> OptOptions) override;

  WasiCryptoExpect<SymmetricKey> import(Span<uint8_t const> Raw) override;

  WasiCryptoExpect<__wasi_size_t> keyLen() override;

private:
  SymmetricAlgorithm Alg;
};

// Nonce = IV,
class AesGcmSymmetricState : public SymmetricState::Base {
public:
  inline static constexpr __wasi_size_t NonceLen = 12;

  inline static constexpr __wasi_size_t TagLen = 16;

  AesGcmSymmetricState(SymmetricAlgorithm Alg, SymmetricOptions Options,
                       AesGcmCtx Ctx)
      : SymmetricState::Base(Alg), Options(Options), Ctx(std::move(Ctx)) {}

  /// There are four inputs for authenticated encryption:
  /// @param[in] OptKey The secret key for encrypt
  /// @param[in] OptOptions `Must` Contain an Nonce(Initialization vector).
  /// Otherwise, generate an Nonce in runtime
  static WasiCryptoExpect<std::unique_ptr<AesGcmSymmetricState>>
  import(SymmetricAlgorithm Alg, std::optional<SymmetricKey> OptKey,
         std::optional<SymmetricOptions> OptOptions);

  WasiCryptoExpect<std::vector<uint8_t>>
  optionsGet(std::string_view Name) override;

  WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) override;

  /// @param[in] optional additional authentication data(AAD)
  WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override;

  WasiCryptoExpect<__wasi_size_t> maxTagLen() override { return TagLen; }

protected:
  /// @param[in] Data The plain text.
  /// @param[out] Out The encrypted text and tag.
  WasiCryptoExpect<__wasi_size_t>
  encryptUnchecked(Span<uint8_t> Out, Span<const uint8_t> Data) override;

  /// @param[in] Data The plain text.
  /// @param[out] Out The encrypted text.
  /// @return Tag.
  WasiCryptoExpect<SymmetricTag>
  encryptDetachedUnchecked(Span<uint8_t> Out,
                           Span<const uint8_t> Data) override;

  /// @param[in] Data The encrypted text and tag.
  /// @param[out] Out The plain text.
  WasiCryptoExpect<__wasi_size_t>
  decryptUnchecked(Span<uint8_t> Out, Span<const uint8_t> Data) override;

  WasiCryptoExpect<__wasi_size_t>
  decryptDetachedUnchecked(Span<uint8_t> Out, Span<const uint8_t> Data,
                           Span<uint8_t const> RawTag) override;

private:
  SymmetricOptions Options;
  AesGcmCtx Ctx;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
