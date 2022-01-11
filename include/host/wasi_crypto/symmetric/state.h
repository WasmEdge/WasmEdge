// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/symmetric/alg.h"
#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/options.h"
#include "host/wasi_crypto/symmetric/tag.h"
#include "wasi_crypto/api.hpp"

#include <memory>
#include <mutex>
#include <optional>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

/// Each algorithm type supports a subset of symmetric_state_*() functions.
///
/// If a function is not defined for an algorithm, it MUST unconditionally
/// return an unsupported_feature error code.
class State {
public:
  virtual ~State() = default;

  static WasiCryptoExpect<std::unique_ptr<State>>
  open(SymmetricAlgorithm Alg, std::shared_ptr<Key> OptKey,
       std::shared_ptr<Options> OptOption);

  virtual WasiCryptoExpect<std::vector<uint8_t>>
  optionsGet(std::string_view Name) = 0;

  virtual WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) = 0;

  virtual WasiCryptoExpect<void> absorb(Span<uint8_t const> Data);

  virtual WasiCryptoExpect<void> squeeze(Span<uint8_t> Out);

  virtual WasiCryptoExpect<void> ratchet();

  virtual WasiCryptoExpect<__wasi_size_t> encrypt(Span<uint8_t> Out,
                                                  Span<uint8_t const> Data);

  virtual WasiCryptoExpect<Tag> encryptDetached(Span<uint8_t> Out,
                                                Span<uint8_t const> Data);

  virtual WasiCryptoExpect<__wasi_size_t> decrypt(Span<uint8_t> Out,
                                                  Span<uint8_t const> Data);

  virtual WasiCryptoExpect<__wasi_size_t>
  decryptDetached(Span<uint8_t> Out, Span<uint8_t const> Data,
                  Span<uint8_t> RawTag);

  virtual WasiCryptoExpect<std::unique_ptr<Key>>
  squeezeKey(SymmetricAlgorithm KeyAlg);

  virtual WasiCryptoExpect<Tag> squeezeTag();

  virtual WasiCryptoExpect<__wasi_size_t> maxTagLen();
};

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
