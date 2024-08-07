// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ctx.h"
#include "signatures/signatures.h"
#include "signatures/signstate.h"
#include "signatures/verificationstate.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

WasiCryptoExpect<__wasi_array_output_t>
Context::signatureExport(__wasi_signature_t SigHandle,
                         __wasi_signature_encoding_e_t Encoding) noexcept {
  return SignatureManager.get(SigHandle)
      .and_then([Encoding](auto &&SigVariant) noexcept {
        return Signatures::sigExportData(
            std::forward<decltype(SigVariant)>(SigVariant), Encoding);
      })
      .and_then([this](auto &&Data) noexcept {
        return ArrayOutputManager.registerManager(
            std::forward<decltype(Data)>(Data));
      });
}

WasiCryptoExpect<void>
Context::signatureClose(__wasi_signature_t SigHandle) noexcept {
  return SignatureManager.close(SigHandle);
}

WasiCryptoExpect<__wasi_array_output_t>
Context::signatureImport(Signatures::Algorithm Alg, Span<const uint8_t> Encoded,
                         __wasi_signature_encoding_e_t Encoding) noexcept {
  return Signatures::sigImport(Alg, Encoded, Encoding)
      .and_then([this](auto &&Sig) noexcept {
        return SignatureManager.registerManager(
            std::forward<decltype(Sig)>(Sig));
      });
}

WasiCryptoExpect<__wasi_signature_state_t>
Context::signatureStateOpen(__wasi_signature_keypair_t KpHandle) noexcept {
  return KeyPairManager.getAs<Signatures::KpVariant>(KpHandle)
      .and_then([](auto &&KpVariant) noexcept {
        return Signatures::sigStateOpen(
            std::forward<decltype(KpVariant)>(KpVariant));
      })
      .and_then([this](auto &&SignStateVariant) noexcept {
        return SignStateManager.registerManager(
            std::forward<decltype(SignStateVariant)>(SignStateVariant));
      });
}

WasiCryptoExpect<void>
Context::signatureStateUpdate(__wasi_signature_state_t StateHandle,
                              Span<const uint8_t> Input) noexcept {
  return SignStateManager.get(StateHandle)
      .and_then([Input](auto &&SignStateVariant) noexcept {
        return Signatures::sigStateUpdate(SignStateVariant, Input);
      });
}

WasiCryptoExpect<__wasi_signature_t>
Context::signatureStateSign(__wasi_signature_state_t StateHandle) noexcept {
  return SignStateManager.get(StateHandle)
      .and_then([](auto &&SignStateVariant) noexcept {
        return Signatures::sigStateSign(SignStateVariant);
      })
      .and_then([this](auto &&Signature) noexcept {
        return SignatureManager.registerManager(
            std::forward<decltype(Signature)>(Signature));
      });
}

WasiCryptoExpect<void>
Context::signatureStateClose(__wasi_signature_state_t StateHandle) noexcept {
  return SignStateManager.close(StateHandle);
}

WasiCryptoExpect<__wasi_signature_verification_state_t>
Context::signatureVerificationStateOpen(
    __wasi_signature_publickey_t PkHandle) noexcept {
  return PublicKeyManager.getAs<Signatures::PkVariant>(PkHandle)
      .and_then([](auto &&PkVariant) noexcept {
        return Signatures::verificationStateOpen(
            std::forward<decltype(PkVariant)>(PkVariant));
      })
      .and_then([this](auto &&VerificationStateVariant) noexcept {
        return VerificationStateManager.registerManager(
            std::forward<decltype(VerificationStateVariant)>(
                VerificationStateVariant));
      });
}

WasiCryptoExpect<void> Context::signatureVerificationStateUpdate(
    __wasi_signature_verification_state_t VerificationHandle,
    Span<const uint8_t> Input) noexcept {
  return VerificationStateManager.get(VerificationHandle)
      .and_then([Input](auto &&VerificationStateVariant) noexcept {
        return Signatures::verificationStateUpdate(VerificationStateVariant,
                                                   Input);
      });
}

WasiCryptoExpect<void> Context::signatureVerificationStateVerify(
    __wasi_signature_verification_state_t VerificationHandle,
    __wasi_signature_t SigHandle) noexcept {
  auto Verification = VerificationStateManager.get(VerificationHandle);
  if (!Verification) {
    return WasiCryptoUnexpect(Verification);
  }

  auto Sig = SignatureManager.get(SigHandle);
  if (!Sig) {
    return WasiCryptoUnexpect(Sig);
  }

  return Signatures::verificationStateVerify(*Verification, *Sig);
}

WasiCryptoExpect<void> Context::signatureVerificationStateClose(
    __wasi_signature_verification_state_t VerificationHandle) noexcept {
  return VerificationStateManager.close(VerificationHandle);
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
