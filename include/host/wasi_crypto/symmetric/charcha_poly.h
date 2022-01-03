// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/expected.h"
#include "experimental/expected.hpp"
#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/state.h"
#include "host/wasi_crypto/wrapper/charcha_poly.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class ChaChaPolySymmetricKey : public SymmetricKey::Base {
public:
  ChaChaPolySymmetricKey(SymmetricAlgorithm Alg, Span<uint8_t const> Raw);

  Span<const uint8_t> asRef() override { return Raw; }

  SymmetricAlgorithm alg() override { return Alg;}

private:
  SymmetricAlgorithm Alg;
  std::vector<uint8_t> Raw;
};

class ChaChaPolySymmetricKeyBuilder : public SymmetricKey::Builder {
public:
  ChaChaPolySymmetricKeyBuilder(SymmetricAlgorithm Alg);

  WasiCryptoExpect<SymmetricKey>
  generate(std::optional<SymmetricOptions> OptOption) override;

  WasiCryptoExpect<SymmetricKey> import(Span<uint8_t const> Raw) override;

  WasiCryptoExpect<__wasi_size_t> keyLen() override;

private:
  SymmetricAlgorithm Alg;
};

class ChaChaPolySymmetricState : public SymmetricState::Base {
public:
  static WasiCryptoExpect<std::unique_ptr<ChaChaPolySymmetricState>>
  make(SymmetricAlgorithm Alg, std::optional<SymmetricKey> OptKey,
       std::optional<SymmetricOptions> OptOptions);

  WasiCryptoExpect<std::vector<uint8_t>>
  optionsGet(std::string_view Name) override;

  WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) override;

  /// @param[in] optional additional authentication data(AAD)
  WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override;


protected:
  /// @param[out] Out The encrypted text and tag.
  /// @param[in] Data The plain text.
  WasiCryptoExpect<__wasi_size_t>
  encryptUnchecked(Span<uint8_t> Out, Span<const uint8_t> Data) override;

  /// @param[out] Out The encrypted text.
  /// @param[in] Data The plain text.
  /// @return Tag.
  WasiCryptoExpect<SymmetricTag>
  encryptDetachedUnchecked(Span<uint8_t> Out,
                           Span<const uint8_t> Data) override;

  /// @param[out] Out The plain text.
  /// @param[in] Data The encrypted text and tag.
  WasiCryptoExpect<__wasi_size_t>
  decryptUnchecked(Span<uint8_t> Out, Span<const uint8_t> Data) override;

  /// @param[out] Out The plain text.
  /// @param[in] Data The encrypted text.
  /// @param[in] RawTag Tag.
  WasiCryptoExpect<__wasi_size_t>
  decryptDetachedUnchecked(Span<uint8_t> Out, Span<const uint8_t> Data,
                           Span<uint8_t const> RawTag) override;

private:
  SymmetricOptions::Inner Options;
  ChaChaPolyCtx Ctx;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
