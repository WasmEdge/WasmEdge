// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/symmetric/state.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

/// If finalization is required, the implementation MUST duplicate the internal
/// state and apply the finalization on the copy, leaving the state unchanged
/// from the guest perspective. This does not apply to hash functions designed
/// to include squeeze() calls in a session transcript.
///
/// Implementations MUST support an arbitrary number of absorb() and squeeze()
/// calls, in any order.
class HashState : public State {
public:
  /// Adds input data to the state, and is equivalent to the
  /// UPDATE() function in NIST interfaces.
  ///
  /// @param[in] Data Input data
  /// @return Nothing or WasiCrypto error
  virtual WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override = 0;

  /// Returns a digest of the input received up to the function
  /// call.
  ///
  /// The output buffer given to the squeeze function can be smaller than the
  /// hash function output size. In that case, the implementation MUST truncate
  /// the output to the requested length.
  ///
  /// @param[out] Out the output buffer of digest
  /// @return If the requested size exceeds what the hash function
  /// can output, the `__WASI_CRYPTO_ERRNO_INVALID_LENGTH` error code MUST be
  /// returned.
  virtual WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) override = 0;

public:
  WasiCryptoExpect<void> ratchet() override { return State::ratchet(); }
  
  WasiCryptoExpect<__wasi_size_t> encrypt(Span<uint8_t> Out,
                                          Span<const uint8_t> Data) override {
    return State::encrypt(Out, Data);
  }
  WasiCryptoExpect<Tag> encryptDetached(Span<uint8_t> Out,
                                        Span<const uint8_t> Data) override {
    return State::encryptDetached(Out, Data);
  }
  WasiCryptoExpect<__wasi_size_t> decrypt(Span<uint8_t> Out,
                                          Span<const uint8_t> Data) override {
    return State::decrypt(Out, Data);
  }
  WasiCryptoExpect<__wasi_size_t>
  decryptDetached(Span<uint8_t> Out, Span<const uint8_t> Data,
                  Span<uint8_t> RawTag) override {
    return State::decryptDetached(Out, Data, RawTag);
  }
  WasiCryptoExpect<Key> squeezeKey(SymmetricAlgorithm KeyAlg) override {
    return State::squeezeKey(KeyAlg);
  }
  WasiCryptoExpect<Tag> squeezeTag() override { return State::squeezeTag(); }
  WasiCryptoExpect<__wasi_size_t> maxTagLen() override {
    return State::maxTagLen();
  }
};

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
