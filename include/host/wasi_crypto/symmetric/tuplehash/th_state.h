// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/symmetric/state.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

class TupleHashState : public State {
public:
  /// Individual inputs to the absorb() function
  /// MUST be domain separated and MUST NOT be concatenated.
  ///
  /// @param[in] Data Input data
  /// @return Nothing or WasiCrypto error
  virtual WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) = 0;

  /// @param[out] Out the output buffer of digest
  /// @return If the requested size exceeds what the hash function
  /// can output, the `__WASI_CRYPTO_ERRNO_INVALID_LENGTH` error code MUST be
  /// returned.
  virtual WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) = 0;
};

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
