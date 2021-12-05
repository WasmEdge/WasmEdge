// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/rsa.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
WasiCryptoExpect<void>
RsaSignatureVerificationState::update(Span<const uint8_t> Input) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}
WasiCryptoExpect<void> RsaSignatureVerificationState::verify(Signature &Sig) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
