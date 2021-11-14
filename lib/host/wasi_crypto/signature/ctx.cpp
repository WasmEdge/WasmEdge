// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/ctx.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<__wasi_array_output_t>
WASICrypto::SignatureContext::signatureExport(
    __wasi_signature_t Signature,
    __wasi_signature_encoding_e_t SignatureEncoding) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_array_output_t>();
}

WasiCryptoExpect<void> SignatureContext::signatureVerificationStateUpdate(
    __wasi_signature_verification_state_t State, Span<const uint8_t> Input) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
