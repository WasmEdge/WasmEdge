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
#include "host/wasi_crypto/wrapper/eddsa.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class EddsaSignaturePublicKey : public SignaturePublicKey::Base {
public:
  EddsaSignaturePublicKey(EddsaPkCtx Ctx) : Ctx(std::move(Ctx)) {}

  static WasiCryptoExpect<std::unique_ptr<EddsaSignaturePublicKey>>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_publickey_encoding_e_t Encoding) override;

  WasiCryptoExpect<SignatureVerificationState> asState() override;

private:
  EddsaPkCtx Ctx;
};

class EddsaSignatureSecretKey : public SignatureSecretKey::Base {
public:
  EddsaSignatureSecretKey(EddsaSkCtx Ctx) : Ctx(std::move(Ctx)) {}

  static WasiCryptoExpect<SignatureSecretKey>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_secretkey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_secretkey_encoding_e_t Encoding) override;

private:
  EddsaSkCtx Ctx;
};

class EddsaSignatureKeyPair : public SignatureKeyPair::Base {
public:
  EddsaSignatureKeyPair(EddsaKpCtx Ctx) : Ctx(std::move(Ctx)) {}

  static WasiCryptoExpect<SignatureKeyPair>
  generate(SignatureAlgorithm Alg, std::optional<SignatureOptions> Options);

  static WasiCryptoExpect<SignatureKeyPair>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_keypair_encoding_e_t Encoding) override;

  WasiCryptoExpect<SignaturePublicKey> publicKey() override;

  WasiCryptoExpect<SignatureSecretKey> secretKey() override;

  WasiCryptoExpect<SignatureState> asState() override;

private:
  EddsaKpCtx Ctx;
};

class EddsaSignature : public Signature::Base {
public:
  EddsaSignature(EddsaSignCtx Ctx) : Ctx(std::move(Ctx)) {}

  static WasiCryptoExpect<Signature>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_signature_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_signature_encoding_e_t Encoding) override;

  Span<uint8_t const> asRef() override { return Ctx.asRef(); }

private:
  EddsaSignCtx Ctx;
};

class EddsaSignatureState : public SignatureState::Base {
public:
  EddsaSignatureState(EddsaSignStateCtx Ctx) : Ctx(std::move(Ctx)) {}

  WasiCryptoExpect<void> update(Span<uint8_t const> Input) override;

  WasiCryptoExpect<Signature> sign() override;

private:
  EddsaSignStateCtx Ctx;
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
