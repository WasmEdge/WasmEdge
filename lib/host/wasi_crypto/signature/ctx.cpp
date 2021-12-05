// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/ctx.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoContext::signatureExport(__wasi_signature_t SigHandle,
                                   __wasi_signature_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW:
    break;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
  auto Sig = SignatureManger.get(SigHandle);
  auto Res = Sig->inner()->locked([](auto&Inner) {
    return Inner->asRaw();
  });
  return allocateArrayOutput(std::move(Res));
}

WasiCryptoExpect<__wasi_signature_t>
WasiCryptoContext::signatureImport(SignatureAlgorithm Alg,
                                   Span<const uint8_t> Encoded,
                                   __wasi_signature_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW:
    break;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }

  auto Sig = Signature::fromRaw(Alg, Encoded);
  if (!Sig) {
    return WasiCryptoUnexpect(Sig);
  }

  return SignatureManger.registerManger(*Sig);
}

WasiCryptoExpect<void>
WasiCryptoContext::signatureClose(__wasi_signature_t SigHandle) {
  return SignatureManger.close(SigHandle);
}

WasiCryptoExpect<__wasi_signature_state_t>
WasiCryptoContext::signatureStateOpen(__wasi_signature_keypair_t KpHandle) {
  auto Kp = KeypairManger.get(KpHandle);
  if (!Kp) {
    return WasiCryptoUnexpect(Kp);
  }

  auto SigKp = Kp->as<SignatureKeyPair>();
  if (!SigKp) {
    return WasiCryptoUnexpect(SigKp);
  }

  auto SigState = SignatureState::open(*SigKp);
  if (!SigState) {
    return WasiCryptoUnexpect(SigState);
  }

  return SignatureStateManger.registerManger(*SigState);
}

WasiCryptoExpect<void>
WasiCryptoContext::signatureStateUpdate(__wasi_signature_state_t StateHandle,
                                        Span<const uint8_t> Input) {
  auto State = SignatureStateManger.get(StateHandle);
  if (!State) {
    return WasiCryptoUnexpect(State);
  }

  return State->update(Input);
}

WasiCryptoExpect<__wasi_signature_t>
WasiCryptoContext::signatureStateSign(__wasi_signature_state_t StateHandle) {
  auto State = SignatureStateManger.get(StateHandle);
  if (!State) {
    return WasiCryptoUnexpect(State);
  }

  auto Sig = State->sign();
  if (!Sig) {
    return WasiCryptoUnexpect(Sig);
  }

  return SignatureManger.registerManger(*Sig);
}

WasiCryptoExpect<void>
WasiCryptoContext::signatureStateClose(__wasi_signature_state_t StateHandle) {
  return SignatureStateManger.close(StateHandle);
}

WasiCryptoExpect<__wasi_signature_verification_state_t>
WasiCryptoContext::signatureVerificationStateOpen(
    __wasi_signature_publickey_t PkHandle) {
  auto Pk = PublickeyManger.get(PkHandle);
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  auto SigPk = Pk->as<SignaturePublicKey>();
  if (!SigPk) {
    return WasiCryptoUnexpect(SigPk);
  }

  auto Verification = SignatureVerificationState::open(*SigPk);
  if (!Verification) {
    return WasiCryptoUnexpect(Verification);
  }

  return SignatureVerificationStateManger.registerManger(*Verification);
}

WasiCryptoExpect<void> WasiCryptoContext::signatureVerificationStateUpdate(
    __wasi_signature_verification_state_t VerificationHandle,
    Span<const uint8_t> Input) {
  auto Verification = SignatureVerificationStateManger.get(VerificationHandle);
  if (!Verification) {
    return WasiCryptoUnexpect(Verification);
  }

  return Verification->update(Input);
}

WasiCryptoExpect<void> WasiCryptoContext::signatureVerificationStateVerify(
    __wasi_signature_verification_state_t VerificationHandle,
    __wasi_signature_t SigHandle) {
  auto Verification = SignatureVerificationStateManger.get(VerificationHandle);
  if (!Verification) {
    return WasiCryptoUnexpect(Verification);
  }

  auto Sig = SignatureManger.get(SigHandle);
  if (!Sig) {
    return WasiCryptoUnexpect(Sig);
  }

  return Verification->verify(*Sig);
}

WasiCryptoExpect<void> WasiCryptoContext::signatureVerificationStateClose(
    __wasi_signature_verification_state_t VerificationHandle) {
  return SignatureVerificationStateManger.close(VerificationHandle);
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
