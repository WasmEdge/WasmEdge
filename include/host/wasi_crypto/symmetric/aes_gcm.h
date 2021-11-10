// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/state.h"
#include "openssl/aes.h"
#include "openssl/evp.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class AesGcmSymmetricKey : public SymmetricKey {
public:
  AesGcmSymmetricKey(SymmetricAlgorithm Alg, Span<uint8_t const> Raw);

  WasiCryptoExpect<Span<uint8_t>> raw() override;

  SymmetricAlgorithm alg() override;

private:
  SymmetricAlgorithm Alg;
  std::vector<uint8_t> Raw;
};

class AesGcmSymmetricKeyBuilder : public SymmetricKeyBuilder {
public:
  AesGcmSymmetricKeyBuilder(SymmetricAlgorithm Alg);

  WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
  generate(std::shared_ptr<SymmetricOption> Option) override;

  WasiCryptoExpect<std::unique_ptr<SymmetricKey>>
  import(Span<uint8_t const> Raw) override;

  WasiCryptoExpect<__wasi_size_t> keyLen() override;

private:
  SymmetricAlgorithm Alg;
};

// Nonce = IV,
class AesGcmSymmetricState : public SymmetricState {
public:
  inline static constexpr __wasi_size_t NonceLen = 12;

  inline static constexpr __wasi_size_t TagLen = 16;

  /// There are four inputs for authenticated encryption:
  /// @param[in] Key The secret key for encrypt
  /// @param[in] Options `Must` Contain an Nonce(Initialization vector).
  /// Otherwise, generate an Nonce in runtime
  static WasiCryptoExpect<std::unique_ptr<AesGcmSymmetricState>>
  make(SymmetricAlgorithm Algorithm, std::shared_ptr<SymmetricKey> Key,
       std::shared_ptr<SymmetricOption> Options);

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
  AesGcmSymmetricState(
      SymmetricAlgorithm Algorithm, std::shared_ptr<SymmetricOption> Options,
      OpenSSlUniquePtr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free> Ctx);

  OpenSSlUniquePtr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free> Ctx;

  enum Mode { Unchanged = -1, Decrypt = 0, Encrypt = 1 };

  void updateMode(Mode Mo) {
    EVP_CipherInit_ex(Ctx.get(), nullptr, nullptr, nullptr, nullptr, Mo);
  };
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
