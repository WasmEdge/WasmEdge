// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/symmetric/state.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {
/// If finalization is required, the implementation MUST duplicate the internal
/// state and apply the finalization on the copy, leaving the state unchanged
/// from the guest perspective.
class MACState : public State {
public:
  /// Adds input data to the state.
  ///
  /// @param[in] Data Input data
  /// @return Nothing or WasiCrypto error
  virtual WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override = 0;

  /// authenticates the input received up to the function call.
  ///
  ///
  /// If finalization is required, the implementation MUST duplicate the
  /// internal state and apply the finalization on the copy, leaving the state
  /// unchanged from the guest perspective.
  ///
  /// @return Tag or WasiCrypto error
  virtual WasiCryptoExpect<Tag> squeezeTag() override = 0;

private:
  WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) override final {
    return State::squeeze(Out);
  }

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

  WasiCryptoExpect<std::unique_ptr<Key>>
  squeezeKey(SymmetricAlgorithm KeyAlg) override final {
    return State::squeezeKey(KeyAlg);
  }

  WasiCryptoExpect<__wasi_size_t> maxTagLen() override final {
    return State::maxTagLen();
  }
};
} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
