// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/ctx.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoContext::signatureExport(
    __wasi_signature_t Signature,
    __wasi_signature_encoding_e_t SignatureEncoding) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_array_output_t>();
}

WasiCryptoExpect<void> WasiCryptoContext::signatureVerificationStateUpdate(
    __wasi_signature_verification_state_t State, Span<const uint8_t> Input) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}

WasiCryptoExpect<__wasi_signature_t>
WasiCryptoContext::signatureImport(SignatureAlgorithm Alg,
                                   Span<const uint8_t> Encoded) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_signature_t>();
}

WasiCryptoExpect<__wasi_signature_state_t>
WasiCryptoContext::signatureStateOpen(__wasi_signature_keypair_t Kp) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<
      __wasi_signature_state_t>();
}

WasiCryptoExpect<void>
WasiCryptoContext::signatureStateUpdate(__wasi_signature_state_t State,
                                        Span<const uint8_t> Input) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}

WasiCryptoExpect<__wasi_signature_t>
WasiCryptoContext::signatureStateSign(__wasi_signature_state_t State) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_signature_t>();
}

WasiCryptoExpect<void>
WasiCryptoContext::signatureStateClose(__wasi_signature_state_t State) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}

WasiCryptoExpect<__wasi_signature_verification_state_t>
WasiCryptoContext::signatureVerificationStateOpen(
    __wasi_signature_publickey_t Pk) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<
      __wasi_signature_verification_state_t>();
}

WasiCryptoExpect<void> WasiCryptoContext::signatureVerificationStateVerify(
    __wasi_signature_verification_state_t State, __wasi_signature_t Signature) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}

WasiCryptoExpect<void> WasiCryptoContext::signatureVerificationStateClose(
    __wasi_signature_verification_state_t State) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}

WasiCryptoExpect<void>
WasiCryptoContext::signatureClose(__wasi_signature_t State) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
