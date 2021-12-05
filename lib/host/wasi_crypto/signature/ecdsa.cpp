// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/ecdsa.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
WasiCryptoExpect<EcdsaSignaturePublicKey>
EcdsaSignaturePublicKey::import(SignatureAlgorithm Alg,
                                Span<const uint8_t> Encoded,
                                __wasi_publickey_encoding_e_t Encoding) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<
      EcdsaSignaturePublicKey>();
}

WasiCryptoExpect<std::vector<uint8_t>>
EcdsaSignaturePublicKey::exportData(__wasi_publickey_encoding_e_t Encoding) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<std::vector<uint8_t>>();
}

WasiCryptoExpect<void> EcdsaSignatureState::update(Span<uint8_t> Input) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}
WasiCryptoExpect<void> EcdsaSignatureState::sign() {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}
WasiCryptoExpect<void>
EcdsaSignatureVerificationState::update(Span<const uint8_t> Input) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}
WasiCryptoExpect<void> EcdsaSignatureVerificationState::verify(Signature &Sig) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<void>();
}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
