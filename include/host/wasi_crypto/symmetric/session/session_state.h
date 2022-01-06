// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/symmetric/state.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

class SessionState : public State {
public:
  ///  Absorbs additional data. Multiple calls to absorb() MUST be equivalent to
  ///  a single call with a concatenation of the inputs.
  ///
  /// @param Data Additional data
  /// @return Nothing or WasiCrypto error
  virtual WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override = 0;

  virtual WasiCryptoExpect<__wasi_size_t> maxTagLen() override = 0;

  virtual WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) override = 0;

public:
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
                  Span<uint8_t> RawTag) override {
    return State::decryptDetached(Out, Data, RawTag);
  }

  WasiCryptoExpect<std::unique_ptr<Key>> squeezeKey(SymmetricAlgorithm KeyAlg) override final {
    return State::squeezeKey(KeyAlg);
  }

  WasiCryptoExpect<Tag> squeezeTag() override final {
    return State::squeezeTag();
  }
};

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
