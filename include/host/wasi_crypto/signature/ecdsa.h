// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/signature/alg.h"
#include "host/wasi_crypto/signature/keypair.h"
#include "host/wasi_crypto/signature/options.h"
#include "host/wasi_crypto/signature/publickey.h"
#include "host/wasi_crypto/signature/secretkey.h"
#include "host/wasi_crypto/signature/signature.h"
#include "host/wasi_crypto/wrapper/ecdsa.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class EcdsaSignaturePublicKey : public SignaturePublicKey::Base {
public:
  EcdsaSignaturePublicKey(EcdsaPkCtx Ctx, SignatureAlgorithm Alg)
      : Ctx(std::move(Ctx)), Alg(Alg) {}

  static WasiCryptoExpect<std::unique_ptr<EcdsaSignaturePublicKey>>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_publickey_encoding_e_t Encoding);

  SignatureAlgorithm alg() override { return Alg; }

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_publickey_encoding_e_t Encoding) override;

  WasiCryptoExpect<SignatureVerificationState> asState() override;

private:
  EcdsaPkCtx Ctx;
  SignatureAlgorithm Alg;
};

class EcdsaSignatureSecretKey : public SignatureSecretKey::Base {
public:
  EcdsaSignatureSecretKey(EcdsaSkCtx Ctx, SignatureAlgorithm Alg)
      : Ctx(std::move(Ctx)), Alg(Alg) {}

  static WasiCryptoExpect<std::unique_ptr<EcdsaSignatureSecretKey>>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_secretkey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_secretkey_encoding_e_t Encoding) override;

private:
  EcdsaSkCtx Ctx;
  SignatureAlgorithm Alg;
};

class EcdsaSignatureKeyPair : public SignatureKeyPair::Base {
public:
  EcdsaSignatureKeyPair(EcdsaKpCtx Ctx, SignatureAlgorithm Alg) : Ctx(std::move(Ctx)), Alg(Alg) {}

  static WasiCryptoExpect<std::unique_ptr<EcdsaSignatureKeyPair>>
  generate(SignatureAlgorithm Alg, std::optional<SignatureOptions> Options);

  static WasiCryptoExpect<std::unique_ptr<EcdsaSignatureKeyPair>>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_keypair_encoding_e_t Encoding) override;

  WasiCryptoExpect<SignaturePublicKey> publicKey() override;

  WasiCryptoExpect<SignatureSecretKey> secretKey() override;

  WasiCryptoExpect<SignatureState> asState() override;

private:
  EcdsaKpCtx Ctx;
  SignatureAlgorithm Alg;
};

class EcdsaSignature : public Signature::Base {
public:
  EcdsaSignature(std::vector<uint8_t> &&Raw) : Raw(Raw) {}

  static WasiCryptoExpect<std::unique_ptr<EcdsaSignature>>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_signature_encoding_e_t Encoding);

  Span<uint8_t const> asRef() override { return Raw; }

private:
  std::vector<uint8_t> Raw;
};

class EcdsaSignatureState : public SignatureState::Base {
public:
  EcdsaSignatureState(EcdsaSignCtx Ctx) : Ctx(std::move(Ctx)) {}

  WasiCryptoExpect<void> update(Span<uint8_t const> Input) override;

  WasiCryptoExpect<Signature> sign() override;

private:
  EcdsaSignCtx Ctx;
};

class EcdsaSignatureVerificationState
    : public SignatureVerificationState::Base {
public:
  EcdsaSignatureVerificationState(EcdsaVerificationCtx Ctx)
      : Ctx(std::move(Ctx)){};

  WasiCryptoExpect<void> update(Span<const uint8_t> Input) override;

  WasiCryptoExpect<void> verify(std::unique_ptr<Signature::Base> &Sig) override;

private:
  EcdsaVerificationCtx Ctx;
};
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
