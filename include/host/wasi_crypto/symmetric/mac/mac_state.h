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
  virtual WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) = 0;

  /// authenticates the input received up to the function call.
  ///
  /// @return Tag or WasiCrypto error
  virtual WasiCryptoExpect<Tag> squeezeTag() = 0;
};
}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
