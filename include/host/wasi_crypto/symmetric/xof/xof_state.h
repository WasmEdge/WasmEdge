// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/symmetric/state.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {
class XOFState : public State {
public:
  /// Adds input data to the state.
  ///
  /// @param[in] Data Input data
  /// @return Nothing or WasiCrypto error
  virtual WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) = 0;

  /// wasi-crypto implementations MUST NOT copy the state. Repeated calls to the
  /// squeeze() function MUST produce different outputs.
  ///
  /// @param[out] Out the output buffer
  /// @return Nothing or WasiCrypto error
  virtual WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) = 0;
};

}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
