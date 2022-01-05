// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/symmetric/state.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

// TODO:
/// Password hashing functions MUST support the following operation:
///
/// absorb()
///
/// As well as at least one of the following operations:
///
/// squeeze()
/// squeeze_tag()
///
/// Implementations SHOULD NOT allocate host memory for memory-hard functions.
/// Instead, they SHOULD require guests to provide a scratch buffer, specified
/// as an option named `memory`.
class PasswordHashState : public State {
public:
  /// Absorbs the low-entropy input.
  ///
  /// @param Data Input data
  /// @return Nothing or WasiCrypto error
  virtual WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) = 0;

  /// Returns a derived key stream when the function is used as a KDF
  ///
  /// @param[out] Out the output buffer
  /// @return Nothing or WasiCrypto error
  virtual WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) = 0;

  /// Returns a string that can be used to verify the input. The algorithm must
  /// define a standard representation of such string.
  ///
  /// @return Tag used to verify
  virtual WasiCryptoExpect<Tag> squeezeTag() = 0;
};
}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
