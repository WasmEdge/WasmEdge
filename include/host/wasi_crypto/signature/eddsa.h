// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/signature/alg.h"
#include "host/wasi_crypto/signature/options.h"
#include "host/wasi_crypto/signature/signature.h"
#include "host/wasi_crypto/signature/keypair.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class EddsaSignaturePublicKey : public SignaturePublicKey::Base {
public:
  EddsaSignaturePublicKey(EddsaPkCtx Ctx, SignatureAlgorithm Alg)
      : Ctx(std::move(Ctx)), Alg(Alg) {}

  static WasiCryptoExpect<std::unique_ptr<EddsaSignaturePublicKey>>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_publickey_encoding_e_t Encoding);

  SignatureAlgorithm alg() override { return Alg; }

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_publickey_encoding_e_t Encoding) override;

  WasiCryptoExpect<SignatureVerificationState> asState() override;

private:
  EddsaPkCtx Ctx;
  SignatureAlgorithm Alg;
};

class EddsaSignatureSecretKey : public SignatureSecretKey::Base {
public:
  EddsaSignatureSecretKey(EddsaSkCtx Ctx, SignatureAlgorithm Alg)
      : Ctx(std::move(Ctx)), Alg(Alg) {}

  static WasiCryptoExpect<std::unique_ptr<EddsaSignatureSecretKey>>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_secretkey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_secretkey_encoding_e_t Encoding) override;

private:
  EddsaSkCtx Ctx;
  SignatureAlgorithm Alg;
};

class EddsaSignatureKeyPair : public SignatureKeyPair::Base {
public:
  EddsaSignatureKeyPair(EddsaKpCtx Ctx, SignatureAlgorithm Alg) : Ctx(std::move(Ctx)), Alg(Alg) {}

  static WasiCryptoExpect<std::unique_ptr<EddsaSignatureKeyPair>>
  generate(SignatureAlgorithm Alg, std::optional<SignatureOptions> Options);

  static WasiCryptoExpect<std::unique_ptr<EddsaSignatureKeyPair>>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_keypair_encoding_e_t Encoding) override;

  WasiCryptoExpect<SignaturePublicKey> publicKey() override;

  WasiCryptoExpect<SignatureSecretKey> secretKey() override;

  WasiCryptoExpect<SignatureState> asState() override;

private:
  EddsaKpCtx Ctx;
  SignatureAlgorithm Alg;
};

class EddsaSignature : public Signature::Base {
public:
  EddsaSignature(std::vector<uint8_t> &&Raw) : Raw(Raw) {}

  static WasiCryptoExpect<std::unique_ptr<EddsaSignature>>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_signature_encoding_e_t Encoding);

  Span<uint8_t const> asRef() override { return Raw; }

private:
  std::vector<uint8_t> Raw;
};

class EddsaSignatureState : public SignatureState::Base {
public:
  EddsaSignatureState(EddsaSignCtx Ctx) : Ctx(std::move(Ctx)) {}

  WasiCryptoExpect<void> update(Span<uint8_t const> Input) override;

  WasiCryptoExpect<Signature> sign() override;

private:
  EddsaSignCtx Ctx;
};

class EddsaSignatureVerificationState
    : public SignatureVerificationState::Base {
public:
  EddsaSignatureVerificationState(EddsaVerificationCtx Ctx)
      : Ctx(std::move(Ctx)){};

  WasiCryptoExpect<void> update(Span<const uint8_t> Input) override;

  WasiCryptoExpect<void> verify(std::unique_ptr<Signature::Base> &Sig) override;

private:
  EddsaVerificationCtx Ctx;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
