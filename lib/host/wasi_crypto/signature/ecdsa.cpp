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

  return std::make_unique<EcdsaSignaturePublicKey>(std::move(*Pk), Alg);
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

  return SignatureVerificationState{std::make_unique<EcdsaSignatureVerificationState>(std::move(*Res))};
}

WasiCryptoExpect<std::unique_ptr<EcdsaSignatureSecretKey>>
EcdsaSignatureSecretKey::import(SignatureAlgorithm Alg,
                                Span<const uint8_t> Encoded,
                                __wasi_secretkey_encoding_e_t Encoding) {
  auto Sk = EcdsaSkCtx::import(Alg, Encoded, Encoding);
  if (!Sk) {
    return WasiCryptoUnexpect(Sk);
  }

  return std::make_unique<EcdsaSignatureSecretKey>(std::move(*Sk), Alg);
}

WasiCryptoExpect<std::vector<uint8_t>>
EcdsaSignatureSecretKey::exportData(__wasi_secretkey_encoding_e_t Encoding) {
  return Ctx.exportData(Encoding);
}

WasiCryptoExpect<std::unique_ptr<EcdsaSignatureKeyPair>>
EcdsaSignatureKeyPair::generate(SignatureAlgorithm Alg,
                                std::optional<SignatureOptions>) {
  auto Res = EcdsaKpCtx::generate(Alg);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return std::make_unique<EcdsaSignatureKeyPair>(std::move(*Res), Alg);
}

WasiCryptoExpect<std::unique_ptr<EcdsaSignatureKeyPair>>
EcdsaSignatureKeyPair::import(SignatureAlgorithm Alg,
                              Span<const uint8_t> Encoded,
                              __wasi_keypair_encoding_e_t Encoding) {
  auto Res = EcdsaKpCtx::import(Alg, Encoded, Encoding);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return std::make_unique<EcdsaSignatureKeyPair>(std::move(*Res), Alg);
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
      std::make_unique<EcdsaSignaturePublicKey>(std::move(*Res), Alg)};
}

WasiCryptoExpect<SignatureSecretKey> EcdsaSignatureKeyPair::secretKey() {
  auto Res = Ctx.secretKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureSecretKey{
      std::make_unique<EcdsaSignatureSecretKey>(std::move(*Res), Alg)};
}

WasiCryptoExpect<SignatureState>
EcdsaSignatureKeyPair::asState() {
  auto Res = Ctx.asSign();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureState{std::make_unique<EcdsaSignatureState>(std::move(*Res))};
}

template <__wasi_signature_encoding_e_t Encoding>
WasiCryptoExpect<std::unique_ptr<EcdsaSignature>>
makeFrom(SignatureAlgorithm Alg, Span<uint8_t const>);

template <>
WasiCryptoExpect<std::unique_ptr<EcdsaSignature>>
makeFrom<__WASI_SIGNATURE_ENCODING_RAW>(SignatureAlgorithm Alg,
                                        Span<uint8_t const> Raw) {
  size_t ExpectedSize;
  switch (Alg) {
  case SignatureAlgorithm::ECDSA_K256_SHA256: {
    ExpectedSize = 64;
  }
  case SignatureAlgorithm::ECDSA_P256_SHA256: {
    ExpectedSize = 96;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_SIGNATURE);
  }
  if (Raw.size() != ExpectedSize) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_SIGNATURE);
  }
  return std::make_unique<EcdsaSignature>(
      std::vector<uint8_t>{Raw.begin(), Raw.end()});
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

// template <>
// WasiCryptoExpect<std::unique_ptr<EcdsaSignature m>>
// makeFrom<__WASI_SIGNATURE_ENCODING_DER>(SignatureAlgorithm Alg,
//                                         Span<uint8_t const> Der) {
//   return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
// }

WasiCryptoExpect<std::unique_ptr<EcdsaSignature>>
EcdsaSignature::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                       __wasi_signature_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW:
    return makeFrom<__WASI_SIGNATURE_ENCODING_RAW>(Alg, Encoded);
  case __WASI_SIGNATURE_ENCODING_DER:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    //    return makeFrom<__WASI_SIGNATURE_ENCODING_DER>(Alg, Encoded);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  }
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
