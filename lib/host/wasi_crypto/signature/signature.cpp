// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/signature.h"
#include "host/wasi_crypto/signature/ecdsa.h"
#include "host/wasi_crypto/signature/eddsa.h"
#include "host/wasi_crypto/signature/keypair.h"
#include "host/wasi_crypto/signature/publickey.h"
#include "host/wasi_crypto/signature/rsa.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<Signature> Signature::fromRaw(SignatureAlgorithm Alg,
                                               Span<const uint8_t> Encoded) {
  switch (family(Alg)) {
  case SignatureAlgorithmFamily::ECDSA: {
    auto Sig = EcdsaSignature::fromRaw(Encoded);
    if (!Sig) {
      return WasiCryptoUnexpect(Sig);
    }
    return Signature{std::move(*Sig)};
  }
  case SignatureAlgorithmFamily::EdDSA: {
    auto Sig = EddsaSignature::fromRaw(Encoded);
    if (!Sig) {
      return WasiCryptoUnexpect(Sig);
    }
    return Signature{std::move(*Sig)};
  }
  case SignatureAlgorithmFamily::RSA: {
    auto Sig = RsaSignature::fromRaw(Encoded);
    if (!Sig) {
      return WasiCryptoUnexpect(Sig);
    }
    return Signature{std::move(*Sig)};
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
}

WasiCryptoExpect<SignatureState> SignatureState::open(SignatureKeyPair Kp) {
  return Kp.inner()->locked(
      [](auto &KpInner) -> WasiCryptoExpect<SignatureState> {
        if (dynamic_cast<EcdsaSignatureKeyPair *>(KpInner.get()) != nullptr) {
          return SignatureState{std::move(KpInner)};
        }
        if (dynamic_cast<EddsaSignatureKeyPair *>(KpInner.get()) != nullptr) {
          return SignatureState{std::move(KpInner)};
        }
        if (dynamic_cast<RsaSignatureKeyPair *>(KpInner.get()) != nullptr) {
          return SignatureState{std::move(KpInner)};
        }
        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
      });
}

SignatureVerificationState::SignatureVerificationState(
    std::unique_ptr<Base> Inner)
    : Inner(std::make_shared<Mutex<std::unique_ptr<Base>>>(std::move(Inner))) {}

WasiCryptoExpect<SignatureVerificationState>
SignatureVerificationState::open(SignaturePublicKey SigPk) {
  return SigPk.inner()->locked(
      [](auto &KpInner) -> WasiCryptoExpect<SignatureVerificationState> {
        if (dynamic_cast<EcdsaSignaturePublicKey *>(KpInner.get()) != nullptr) {
          return SignatureVerificationState{std::move(KpInner)};
        }
        if (dynamic_cast<EddsaSignaturePublicKey *>(KpInner.get()) != nullptr) {
          return SignatureVerificationState{std::move(KpInner)};
        }
        if (dynamic_cast<RsaSignaturePublicKey *>(KpInner.get()) != nullptr) {
          return SignatureVerificationState{std::move(KpInner)};
        }
        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
      });
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
