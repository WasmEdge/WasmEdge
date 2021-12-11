// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/eddsa.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<std::unique_ptr<EddsaSignaturePublicKey>>
WASICrypto::EddsaSignaturePublicKey::import(
    SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
    __wasi_publickey_encoding_e_t Encoding) {
  auto Pk = EddsaPkCtx::import(Alg, Encoded, Encoding);
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  return std::make_unique<EddsaSignaturePublicKey>(std::move(*Pk));
}

WasiCryptoExpect<std::vector<uint8_t>>
EddsaSignaturePublicKey::exportData(__wasi_publickey_encoding_e_t Encoding) {
  return Ctx.exportData(Encoding);
}

WasiCryptoExpect<SignatureVerificationState>
EddsaSignaturePublicKey::asState() {
  auto Res = Ctx.asVerification();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureVerificationState{
      std::make_unique<EddsaSignatureVerificationState>(std::move(*Res))};
}

WasiCryptoExpect<SignatureSecretKey>
EddsaSignatureSecretKey::import(SignatureAlgorithm Alg,
                                Span<const uint8_t> Encoded,
                                __wasi_secretkey_encoding_e_t Encoding) {
  auto Sk = EddsaSkCtx::import(Alg, Encoded, Encoding);
  if (!Sk) {
    return WasiCryptoUnexpect(Sk);
  }

  return SignatureSecretKey{std::make_unique<EddsaSignatureSecretKey>(std::move(*Sk))};
}

WasiCryptoExpect<std::vector<uint8_t>>
EddsaSignatureSecretKey::exportData(__wasi_secretkey_encoding_e_t Encoding) {
  return Ctx.exportData(Encoding);
}

WasiCryptoExpect<SignatureKeyPair>
EddsaSignatureKeyPair::generate(SignatureAlgorithm Alg,
                                std::optional<SignatureOptions>) {
  auto Res = EddsaKpCtx::generate(Alg);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureKeyPair{std::make_unique<EddsaSignatureKeyPair>(std::move(*Res))};
}

WasiCryptoExpect<SignatureKeyPair>
EddsaSignatureKeyPair::import(SignatureAlgorithm Alg,
                              Span<const uint8_t> Encoded,
                              __wasi_keypair_encoding_e_t Encoding) {
  auto Res = EddsaKpCtx::import(Alg, Encoded, Encoding);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureKeyPair{std::make_unique<EddsaSignatureKeyPair>(std::move(*Res))};
}

WasiCryptoExpect<std::vector<uint8_t>>
EddsaSignatureKeyPair::exportData(__wasi_keypair_encoding_e_t Encoding) {
  return Ctx.exportData(Encoding);
}

WasiCryptoExpect<SignaturePublicKey> EddsaSignatureKeyPair::publicKey() {
  auto Res = Ctx.publicKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignaturePublicKey{
      std::make_unique<EddsaSignaturePublicKey>(std::move(*Res))};
}

WasiCryptoExpect<SignatureSecretKey> EddsaSignatureKeyPair::secretKey() {
  auto Res = Ctx.secretKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureSecretKey{
      std::make_unique<EddsaSignatureSecretKey>(std::move(*Res))};
}

WasiCryptoExpect<SignatureState> EddsaSignatureKeyPair::asState() {
  auto Res = Ctx.asSignState();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureState{std::make_unique<EddsaSignatureState>(std::move(*Res))};
}

WasiCryptoExpect<Signature>
EddsaSignature::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                       __wasi_signature_encoding_e_t Encoding) {
  auto Res = EddsaSignCtx::import(Alg, Encoded, Encoding);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return Signature{std::make_unique<EddsaSignature>(std::move(*Res))};
}

WasiCryptoExpect<std::vector<uint8_t>> EddsaSignature::exportData(__wasi_signature_encoding_e_t Encoding) {
  return Ctx.exportData(Encoding);
}

WasiCryptoExpect<void> EddsaSignatureState::update(Span<const uint8_t> Input) {
  return Ctx.update(Input);
}

WasiCryptoExpect<Signature> EddsaSignatureState::sign() {
  auto Res = Ctx.sign();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }
  return Signature{std::make_unique<EddsaSignature>(std::move(*Res))};
}

WasiCryptoExpect<void>
EddsaSignatureVerificationState::update(Span<const uint8_t> Input) {
  return Ctx.update(Input);
}

WasiCryptoExpect<void>
EddsaSignatureVerificationState::verify(std::unique_ptr<Signature::Base> &Sig) {
  return Ctx.verify(Sig->asRef());
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
