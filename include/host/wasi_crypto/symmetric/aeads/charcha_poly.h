// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/expected.h"
#include "experimental/expected.hpp"
#include "host/wasi_crypto/symmetric/aeads/aeads_state.h"
#include "host/wasi_crypto/symmetric/key.h"
#include "openssl/evp.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

class ChaChaPolyKeyBuilder : public Key::Builder {
public:
  ChaChaPolyKeyBuilder(SymmetricAlgorithm Alg);

  WasiCryptoExpect<Key> generate(std::shared_ptr<Options> OptOption) override;

  WasiCryptoExpect<Key> import(Span<uint8_t const> Raw) override;

  WasiCryptoExpect<__wasi_size_t> keyLen() override;

private:
  SymmetricAlgorithm Alg;
};

class ChaChaPolySymmetricState : public AEADsState {
public:
  static WasiCryptoExpect<std::unique_ptr<ChaChaPolySymmetricState>>
  import(SymmetricAlgorithm Alg, std::shared_ptr<Key> OptKey,
       std::shared_ptr<Options> OptOptions);

  ChaChaPolySymmetricState(SymmetricAlgorithm /*Alg*/, EVP_CIPHER_CTX *Ctx)
      : /*Alg(Alg),*/ Ctx(Ctx) {}

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

private:
  Options Options;

  enum Mode { Unchanged = -1, Decrypt = 0, Encrypt = 1 };

  //  SymmetricAlgorithm Alg;
  OpenSSLUniquePtr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free> Ctx;
};

class ChaChaPolyCtx {
public:
  inline static constexpr __wasi_size_t TagLen = 16;

  static WasiCryptoExpect<ChaChaPolyCtx> import(SymmetricAlgorithm Alg,
                                                Span<uint8_t const> Key,
                                                Span<uint8_t const> Nonce);

private:
};

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
