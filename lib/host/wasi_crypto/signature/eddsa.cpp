// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/eddsa.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
WasiCryptoExpect<void> EddsaSignatureState::update(Span<uint8_t> Input) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}
WasiCryptoExpect<void> EddsaSignatureState::sign() {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}
WasiCryptoExpect<void>
EddsaSignatureVerificationState::update(Span<const uint8_t> Input) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}
WasiCryptoExpect<void> EddsaSignatureVerificationState::verify(Signature &Sig) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
