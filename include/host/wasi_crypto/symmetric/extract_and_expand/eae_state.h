// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/symmetric/state.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

class ExtractAndExpandState : public State {
public:
  /// Absorbs the salt of the key information. The absence of an absorb() call
  /// MUST be equivalent the an empty salt or key information. Multiple calls to
  /// absorb() MUST be equivalent to a single call with the concatenation of the
  /// inputs.
  ///
  /// @param[in] Data Input data
  /// @return Nothing or WasiCrypto error
  virtual WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override = 0;

  /// @param[out] Out the output buffer
  /// @return If the requested size exceeds what the hash function
  /// can output, the `__WASI_CRYPTO_ERRNO_INVALID_LENGTH` error code MUST be
  /// returned.
  virtual WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) override = 0;

  /// @param[in] Alg Key's Alg
  /// @return returns the PRK, whose algorithm type is set to the EXPAND
  /// counterpart of the EXTRACT operation.
  virtual WasiCryptoExpect<std::unique_ptr<Key>>
  squeezeKey(SymmetricAlgorithm KeyAlg) override = 0;

private:
  WasiCryptoExpect<void> ratchet() override final { return State::ratchet(); }

  WasiCryptoExpect<__wasi_size_t>
  encrypt(Span<uint8_t> Out, Span<const uint8_t> Data) override final {
    return State::encrypt(Out, Data);
  }
  WasiCryptoExpect<Tag>
  encryptDetached(Span<uint8_t> Out, Span<const uint8_t> Data) override final {
    return State::encryptDetached(Out, Data);
  }
  WasiCryptoExpect<__wasi_size_t>
  decrypt(Span<uint8_t> Out, Span<const uint8_t> Data) override final {
    return State::decrypt(Out, Data);
  }
  WasiCryptoExpect<__wasi_size_t>
  decryptDetached(Span<uint8_t> Out, Span<const uint8_t> Data,
                  Span<uint8_t> RawTag) override final {
    return State::decryptDetached(Out, Data, RawTag);
  }
  WasiCryptoExpect<Tag> squeezeTag() override final {
    return State::squeezeTag();
  }
  WasiCryptoExpect<__wasi_size_t> maxTagLen() override final {
    return State::maxTagLen();
  }
};
} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
