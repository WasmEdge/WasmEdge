// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/symmetric/aeads/aeads_state.h"
#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/tag.h"
#include "host/wasi_crypto/wrapper/aes_gcm.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {
using EvpCipherCtxPtr = OpenSSLUniquePtr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free>;


class AesGcmKeyBuilder : public Key::Builder {
public:
  AesGcmKeyBuilder(SymmetricAlgorithm Alg);

  WasiCryptoExpect<Key> generate(std::shared_ptr<Options> OptOptions) override;

  WasiCryptoExpect<Key> import(Span<uint8_t const> Raw) override;

  WasiCryptoExpect<__wasi_size_t> keyLen() override;

private:
  SymmetricAlgorithm Alg;
};

// Nonce = IV,
class AesGcmState : public AEADsState {
public:
  inline static constexpr __wasi_size_t TagLen = 16;

  AesGcmState(SymmetricAlgorithm /*Alg*/, EVP_CIPHER_CTX *Ctx)
        : /*Alg(Alg),*/ Ctx(std::move(Ctx)) {}

  AesGcmState(SymmetricAlgorithm Alg, Options::Inner Options,
                       EVP_CIPHER_CTX *Ctx)
      : SymmetricState::Base(Alg), Options(Options), Ctx(Ctx) {}

  /// There are four inputs for authenticated encryption:
  /// @param[in] OptKey The secret key for encrypt
  /// @param[in] OptOptions `Must` contain an Nonce(Initialization vector).
  static WasiCryptoExpect<std::unique_ptr<AesGcmState>>
  import(SymmetricAlgorithm Alg, std::shared_ptr<Key> OptKey,
         std::shared_ptr<Options> OptOptions);

  WasiCryptoExpect<std::vector<uint8_t>>
  optionsGet(std::string_view Name) override;

  WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) override;

  /// @param[in] optional additional authentication data(AAD)
  WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override;

  WasiCryptoExpect<__wasi_size_t> maxTagLen() override { return TagLen; }

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
  WasiCryptoExpect<__wasi_size_t>
  decryptDetachedUnchecked(Span<uint8_t> Out, Span<const uint8_t> Data,
                           Span<uint8_t const> RawTag) override;

private:
  Options Options;

  enum Mode { Unchanged = -1, Decrypt = 0, Encrypt = 1 };

  //  SymmetricAlgorithm Alg;
  EvpCipherCtxPtr Ctx;
};

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
