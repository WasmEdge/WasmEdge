// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/signature.h"
#include "host/wasi_crypto/signature/keypair.h"
#include "host/wasi_crypto/signature/publickey.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

Signature::Signature(std::unique_ptr<Base> Inner)
    : Inner(std::make_shared<Mutex<std::unique_ptr<Base>>>(std::move(Inner))) {}

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
    auto Sig = EcdsaSignature::fromRaw(Encoded);
    if (!Sig) {
      return WasiCryptoUnexpect(Sig);
    }
    return Signature{std::move(*Sig)};
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
}

SignatureState::SignatureState(std::unique_ptr<Base> Inner)
    : Inner(std::make_shared<Mutex<std::unique_ptr<Base>>>(std::move(Inner))) {}

WasiCryptoExpect<SignatureState> SignatureState::open(SignatureKeyPair Kp) {
  if (auto Res = Kp.as<EcdsaSignatureKeyPair>(); !Res) {
    return WasiCryptoUnexpect(Res);
  } else {
    return SignatureState{std::make_unique<EcdsaSignatureState>(*Res)};
  }

  if (auto Res = Kp.as<EddsaSignatureKeyPair>(); !Res) {
    return WasiCryptoUnexpect(Res);
  } else {
    return SignatureState{std::make_unique<EddsaSignatureState>(*Res)};
  }

  if (auto Res = Kp.as<RsaSignatureKeyPair>(); !Res) {
    return WasiCryptoUnexpect(Res);
  } else {
    return SignatureState{std::make_unique<RsaSignatureState>(*Res)};
  }

  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
}

SignatureVerificationState::SignatureVerificationState(
    std::unique_ptr<Base> Inner)
    : Inner(std::make_shared<Mutex<std::unique_ptr<Base>>>(std::move(Inner))) {}

WasiCryptoExpect<SignatureVerificationState>
SignatureVerificationState::open(SignaturePublicKey SigPk) {
  if (auto Res = SigPk.as<EcdsaSignaturePublicKey>(); !Res) {
    return WasiCryptoUnexpect(Res);
  } else {
    return SignatureVerificationState{
        std::make_unique<EcdsaSignatureVerificationState>(*Res)};
  }

  if (auto Res = SigPk.as<EddsaSignaturePublicKey>(); !Res) {
    return WasiCryptoUnexpect(Res);
  } else {
    return SignatureVerificationState{
        std::make_unique<EddsaSignatureVerificationState>(*Res)};
  }

  if (auto Res = SigPk.as<RsaSignaturePublicKey>(); !Res) {
    return WasiCryptoUnexpect(Res);
  } else {
    return SignatureVerificationState{
        std::make_unique<RsaSignatureVerificationState>(*Res)};
  }

  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
