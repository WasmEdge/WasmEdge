// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "alg.h"
#include "common/span.h"
#include "host/wasi_crypto/common/ctx.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/handles.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
class SignatureContext {
public:
  WasiCryptoExpect<__wasi_array_output_t>
  signatureExport(__wasi_signature_t Signature,
                  __wasi_signature_encoding_e_t SignatureEncoding);

  WasiCryptoExpect<__wasi_signature_t>
  signatureImport(SignatureAlgorithm Alg, Span<uint8_t const> Encoded);

  WasiCryptoExpect<__wasi_signature_state_t>
  signatureStateOpen(__wasi_signature_keypair_t Kp);

  WasiCryptoExpect<void> signatureStateUpdate(__wasi_signature_state_t State,
                                              Span<uint8_t const> Input);

  WasiCryptoExpect<__wasi_signature_t>
  signatureStateSign(__wasi_signature_state_t State);

  WasiCryptoExpect<void> signatureStateClose(__wasi_signature_state_t State);

  WasiCryptoExpect<__wasi_signature_verification_state_t>
  signatureVerificationStateOpen(__wasi_signature_publickey_t Pk);

  WasiCryptoExpect<void>
  signatureVerificationStateUpdate(__wasi_signature_verification_state_t State,
                                   Span<uint8_t const> Input);

  WasiCryptoExpect<void>
  signatureVerificationStateVerify(__wasi_signature_verification_state_t State,
                                   __wasi_signature_t Signature);

  WasiCryptoExpect<void>
  signatureVerificationStateClose(__wasi_signature_verification_state_t State);

  WasiCryptoExpect<void> signatureClose(__wasi_signature_t State);
};
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
