// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/symmetric/aeads/aeads_state.h"
#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/tag.h"

#include "openssl/evp.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

class AesGcmKeyBuilder : public Key::Builder {
public:
  using Builder::Builder;

  WasiCryptoExpect<std::unique_ptr<Key>> generate(std::shared_ptr<Option> OptOption) override;

  WasiCryptoExpect<std::unique_ptr<Key>> import(Span<uint8_t const> Raw) override;

  WasiCryptoExpect<__wasi_size_t> keyLen() override;

private:
  SymmetricAlgorithm Alg;
};

// Nonce = IV,
class AesGcmState : public AEADsState {
public:
  using EvpCipherCtxPtr = OpenSSLUniquePtr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free>;

  inline static constexpr __wasi_size_t TagLen = 16;

  AesGcmState(EVP_CIPHER_CTX *Ctx) : Ctx(Ctx) {}

  /// There are four inputs for authenticated encryption:
  /// @param[in] OptKey The secret key for encrypt
  /// @param[in] OptOption `Must` contain an Nonce(Initialization vector).
  static WasiCryptoExpect<std::unique_ptr<AesGcmState>>
  open(SymmetricAlgorithm Alg, std::shared_ptr<Key> OptKey,
       std::shared_ptr<Option> OptOption);

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
  std::shared_ptr<Option> OptOption;

  enum Mode { Unchanged = -1, Decrypt = 0, Encrypt = 1 };

  //  SymmetricAlgorithm Alg;
  EvpCipherCtxPtr Ctx;
};

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
