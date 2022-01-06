// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/symmetric/aeads/aeads_state.h"
#include "host/wasi_crypto/symmetric/key.h"

#include "openssl/evp.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

class ChaChaPolyKeyBuilder : public Key::Builder {
public:
  using Builder::Builder;

  WasiCryptoExpect<std::unique_ptr<Key>>
  generate(std::shared_ptr<Option> OptOption) override;

  WasiCryptoExpect<std::unique_ptr<Key>>
  import(Span<uint8_t const> Raw) override;

  WasiCryptoExpect<__wasi_size_t> keyLen() override;

private:
  SymmetricAlgorithm Alg;
};

class ChaChaPolyState : public AEADsState {
public:
  ChaChaPolyState(EVP_CIPHER_CTX *Ctx) : Ctx(Ctx) {}

  static WasiCryptoExpect<std::unique_ptr<ChaChaPolyState>>
  open(SymmetricAlgorithm Alg, std::shared_ptr<Key> OptKey,
       std::shared_ptr<Option> OptOptions);

  WasiCryptoExpect<std::vector<uint8_t>>
  optionsGet(std::string_view Name) override;

  WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) override;

  /// @param[in] optional additional authentication data(AAD)
  WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override;

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

public:
  WasiCryptoExpect<__wasi_size_t> maxTagLen() override;

private:
  std::shared_ptr<Option> OptOptions;

  enum Mode { Unchanged = -1, Decrypt = 0, Encrypt = 1 };

  //  SymmetricAlgorithm Alg;
  OpenSSLUniquePtr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free> Ctx;
};

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
