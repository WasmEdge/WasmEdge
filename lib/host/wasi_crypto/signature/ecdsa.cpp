// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/ecdsa.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<std::unique_ptr<EcdsaSignaturePublicKey>>
WASICrypto::EcdsaSignaturePublicKey::import(
    SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
    __wasi_publickey_encoding_e_t Encoding) {
  auto Pk = EcdsaPkCtx::import(Alg, Encoded, Encoding);
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  return std::make_unique<EcdsaSignaturePublicKey>(std::move(*Pk));
}

WasiCryptoExpect<std::vector<uint8_t>>
EcdsaSignaturePublicKey::exportData(__wasi_publickey_encoding_e_t Encoding) {
  return Ctx.exportData(Encoding);
}

WasiCryptoExpect<SignatureVerificationState>
EcdsaSignaturePublicKey::asState() {
  auto Res = Ctx.asVerification();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureVerificationState{
      std::make_unique<EcdsaSignatureVerificationState>(std::move(*Res))};
}

WasiCryptoExpect<SignatureSecretKey>
EcdsaSignatureSecretKey::import(SignatureAlgorithm Alg,
                                Span<const uint8_t> Encoded,
                                __wasi_secretkey_encoding_e_t Encoding) {
  auto Sk = EcdsaSkCtx::import(Alg, Encoded, Encoding);
  if (!Sk) {
    return WasiCryptoUnexpect(Sk);
  }

  return SignatureSecretKey{
      std::make_unique<EcdsaSignatureSecretKey>(std::move(*Sk))};
}

WasiCryptoExpect<std::vector<uint8_t>>
EcdsaSignatureSecretKey::exportData(__wasi_secretkey_encoding_e_t Encoding) {
  return Ctx.exportData(Encoding);
}

WasiCryptoExpect<SignatureKeyPair>
EcdsaSignatureKeyPair::generate(SignatureAlgorithm Alg,
                                std::optional<SignatureOptions>) {
  auto Res = EcdsaKpCtx::generate(Alg);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureKeyPair{
      std::make_unique<EcdsaSignatureKeyPair>(std::move(*Res))};
}

WasiCryptoExpect<SignatureKeyPair>
EcdsaSignatureKeyPair::import(SignatureAlgorithm Alg,
                              Span<const uint8_t> Encoded,
                              __wasi_keypair_encoding_e_t Encoding) {
  auto Res = EcdsaKpCtx::import(Alg, Encoded, Encoding);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureKeyPair{
      std::make_unique<EcdsaSignatureKeyPair>(std::move(*Res))};
}

WasiCryptoExpect<std::vector<uint8_t>>
EcdsaSignatureKeyPair::exportData(__wasi_keypair_encoding_e_t Encoding) {
  return Ctx.exportData(Encoding);
}

WasiCryptoExpect<SignaturePublicKey> EcdsaSignatureKeyPair::publicKey() {
  auto Res = Ctx.publicKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignaturePublicKey{
      std::make_unique<EcdsaSignaturePublicKey>(std::move(*Res))};
}

WasiCryptoExpect<SignatureSecretKey> EcdsaSignatureKeyPair::secretKey() {
  auto Res = Ctx.secretKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureSecretKey{
      std::make_unique<EcdsaSignatureSecretKey>(std::move(*Res))};
}

WasiCryptoExpect<SignatureState> EcdsaSignatureKeyPair::asState() {
  auto Res = Ctx.asSignState();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureState{std::make_unique<EcdsaSignatureState>(std::move(*Res))};
}

WasiCryptoExpect<Signature>
EcdsaSignature::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                       __wasi_signature_encoding_e_t Encoding) {
  auto Res = EcdsaSignCtx::import(Alg, Encoded, Encoding);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return Signature{std::make_unique<EcdsaSignature>(std::move(*Res))};
}

WasiCryptoExpect<std::vector<uint8_t>>
EcdsaSignature::exportData(__wasi_signature_encoding_e_t Encoding) {
  return Ctx.exportData(Encoding);
}

WasiCryptoExpect<void> EcdsaSignatureState::update(Span<const uint8_t> Input) {
  return Ctx.update(Input);
}

WasiCryptoExpect<Signature> EcdsaSignatureState::sign() {
  auto Res = Ctx.sign();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }
  return Signature{std::make_unique<EcdsaSignature>(std::move(*Res))};
}

WasiCryptoExpect<void>
EcdsaSignatureVerificationState::update(Span<const uint8_t> Input) {
  return Ctx.update(Input);
}

WasiCryptoExpect<void>
EcdsaSignatureVerificationState::verify(std::unique_ptr<Signature::Base> &Sig) {
  return Ctx.verify(Sig->asRef());
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
