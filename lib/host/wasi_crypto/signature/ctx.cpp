// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/ctx.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoContext::signatureExport(__wasi_signature_t SigHandle,
                                   __wasi_signature_encoding_e_t Encoding) {
  auto Sig = SignatureManger.get(SigHandle);
  if (!Sig) {
    return WasiCryptoUnexpect(Sig);
  }

  auto Res = (*Sig)->exportData(Encoding);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return allocateArrayOutput(std::move(std::move(*Res)));
}

WasiCryptoExpect<__wasi_signature_t>
WasiCryptoContext::signatureImport(SignatureAlgorithm Alg,
                                   Span<const uint8_t> Encoded,
                                   __wasi_signature_encoding_e_t Encoding) {
  auto Sig = Signatures::Signature::import(Alg, Encoded, Encoding);
  if (!Sig) {
    return WasiCryptoUnexpect(Sig);
  }

  return SignatureManger.registerManger(std::move(*Sig));
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

  auto SigKp = std::visit(
      Overloaded{[](std::shared_ptr<Signatures::KeyPair> Sig)
                     -> WasiCryptoExpect<std::shared_ptr<Signatures::KeyPair>> {
                   return Sig;
                 },
                 [](auto &&)
                     -> WasiCryptoExpect<std::shared_ptr<Signatures::KeyPair>> {
                   return WasiCryptoUnexpect(
                       __WASI_CRYPTO_ERRNO_INVALID_HANDLE);
                 }},
      *Kp);
  if (!SigKp) {
    return WasiCryptoUnexpect(SigKp);
  }

  auto Res = (*SigKp)->openSignState();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureStateManger.registerManger(std::move(*Res));
}

WasiCryptoExpect<void>
WasiCryptoContext::signatureStateUpdate(__wasi_signature_state_t StateHandle,
                                        Span<const uint8_t> Input) {
  auto State = SignatureStateManger.get(StateHandle);
  if (!State) {
    return WasiCryptoUnexpect(State);
  }

  return (*State)->update(Input);
}

WasiCryptoExpect<__wasi_signature_t>
WasiCryptoContext::signatureStateSign(__wasi_signature_state_t StateHandle) {
  auto State = SignatureStateManger.get(StateHandle);
  if (!State) {
    return WasiCryptoUnexpect(State);
  }

  auto Sig = (*State)->sign();
  if (!Sig) {
    return WasiCryptoUnexpect(Sig);
  }

  return SignatureManger.registerManger(std::move(*Sig));
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

  auto SigPk = std::visit(
      Overloaded{
          [](std::shared_ptr<Signatures::PublicKey> Sig)
              -> WasiCryptoExpect<std::shared_ptr<Signatures::PublicKey>> {
            return Sig;
          },
          [](auto &&)
              -> WasiCryptoExpect<std::shared_ptr<Signatures::PublicKey>> {
            return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
          }},
      *Pk);
  if (!SigPk) {
    return WasiCryptoUnexpect(SigPk);
  }

  auto Res = (*SigPk)->openVerificationState();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureVerificationStateManger.registerManger(std::move(*Res));
}

WasiCryptoExpect<void> WasiCryptoContext::signatureVerificationStateUpdate(
    __wasi_signature_verification_state_t VerificationHandle,
    Span<const uint8_t> Input) {
  auto Verification = SignatureVerificationStateManger.get(VerificationHandle);
  if (!Verification) {
    return WasiCryptoUnexpect(Verification);
  }

  return (*Verification)->update(Input);
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

  return (*Verification)->verify(*Sig);
}

WasiCryptoExpect<void> WasiCryptoContext::signatureVerificationStateClose(
    __wasi_signature_verification_state_t VerificationHandle) {
  return SignatureVerificationStateManger.close(VerificationHandle);
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
