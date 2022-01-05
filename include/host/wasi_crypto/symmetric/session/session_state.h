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
  virtual WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) = 0;

  virtual WasiCryptoExpect<__wasi_size_t> maxTagLen() = 0;

  virtual WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) = 0;

};

}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
