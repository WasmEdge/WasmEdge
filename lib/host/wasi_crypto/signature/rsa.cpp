// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/rsa.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<std::unique_ptr<RsaSignaturePublicKey>>
WASICrypto::RsaSignaturePublicKey::import(
    SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
    __wasi_publickey_encoding_e_t Encoding) {
  auto Pk = RsaPkCtx::import(Alg, Encoded, Encoding);
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  return std::make_unique<RsaSignaturePublicKey>(std::move(*Pk));
}

WasiCryptoExpect<std::vector<uint8_t>>
RsaSignaturePublicKey::exportData(__wasi_publickey_encoding_e_t Encoding) {
  return Ctx.exportData(Encoding);
}

WasiCryptoExpect<SignatureVerificationState>
RsaSignaturePublicKey::asState() {
  auto Res = Ctx.asVerification();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureVerificationState{
      std::make_unique<RsaSignatureVerificationState>(std::move(*Res))};
}

WasiCryptoExpect<SignatureSecretKey>
RsaSignatureSecretKey::import(SignatureAlgorithm Alg,
                                Span<const uint8_t> Encoded,
                                __wasi_secretkey_encoding_e_t Encoding) {
  auto Sk = RsaSkCtx::import(Alg, Encoded, Encoding);
  if (!Sk) {
    return WasiCryptoUnexpect(Sk);
  }

  return SignatureSecretKey{std::make_unique<RsaSignatureSecretKey>(std::move(*Sk))};
}

WasiCryptoExpect<std::vector<uint8_t>>
RsaSignatureSecretKey::exportData(__wasi_secretkey_encoding_e_t Encoding) {
  return Ctx.exportData(Encoding);
}

WasiCryptoExpect<SignatureKeyPair>
RsaSignatureKeyPair::generate(SignatureAlgorithm Alg,
                                std::optional<SignatureOptions>) {
  auto Res = RsaKpCtx::generate(Alg);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureKeyPair{std::make_unique<RsaSignatureKeyPair>(std::move(*Res))};
}

WasiCryptoExpect<SignatureKeyPair>
RsaSignatureKeyPair::import(SignatureAlgorithm Alg,
                              Span<const uint8_t> Encoded,
                              __wasi_keypair_encoding_e_t Encoding) {
  auto Res = RsaKpCtx::import(Alg, Encoded, Encoding);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureKeyPair{std::make_unique<RsaSignatureKeyPair>(std::move(*Res))};
}

WasiCryptoExpect<std::vector<uint8_t>>
RsaSignatureKeyPair::exportData(__wasi_keypair_encoding_e_t Encoding) {
  return Ctx.exportData(Encoding);
}

WasiCryptoExpect<SignaturePublicKey> RsaSignatureKeyPair::publicKey() {
  auto Res = Ctx.publicKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignaturePublicKey{
      std::make_unique<RsaSignaturePublicKey>(std::move(*Res))};
}

WasiCryptoExpect<SignatureSecretKey> RsaSignatureKeyPair::secretKey() {
  auto Res = Ctx.secretKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureSecretKey{
      std::make_unique<RsaSignatureSecretKey>(std::move(*Res))};
}

WasiCryptoExpect<SignatureState> RsaSignatureKeyPair::asState() {
  auto Res = Ctx.asSignState();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureState{std::make_unique<RsaSignatureState>(std::move(*Res))};
}

WasiCryptoExpect<Signature>
RsaSignature::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                       __wasi_signature_encoding_e_t Encoding) {
  auto Res = RsaSignCtx::import(Alg, Encoded, Encoding);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return Signature{std::make_unique<RsaSignature>(std::move(*Res))};
}

WasiCryptoExpect<std::vector<uint8_t>> RsaSignature::exportData(__wasi_signature_encoding_e_t Encoding) {
  return Ctx.exportData(Encoding);
}

WasiCryptoExpect<void> RsaSignatureState::update(Span<const uint8_t> Input) {
  return Ctx.update(Input);
}

WasiCryptoExpect<Signature> RsaSignatureState::sign() {
  auto Res = Ctx.sign();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }
  return Signature{std::make_unique<RsaSignature>(std::move(*Res))};
}

WasiCryptoExpect<void>
RsaSignatureVerificationState::update(Span<const uint8_t> Input) {
  return Ctx.update(Input);
}

WasiCryptoExpect<void>
RsaSignatureVerificationState::verify(std::unique_ptr<Signature::Base> &Sig) {
  return Ctx.verify(Sig->asRef());
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
