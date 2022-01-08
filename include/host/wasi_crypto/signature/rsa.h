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
#include "host/wasi_crypto/wrapper/rsa.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class RsaSignaturePublicKey : public SignaturePublicKey::Base {
public:
  RsaSignaturePublicKey(RsaPkCtx Ctx) : Ctx(std::move(Ctx)) {}

  static WasiCryptoExpect<std::unique_ptr<RsaSignaturePublicKey>>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_publickey_encoding_e_t Encoding) override;

  WasiCryptoExpect<SignatureVerificationState> asState() override;

private:
  RsaPkCtx Ctx;
};

class RsaSignatureSecretKey : public SignatureSecretKey::Base {
public:
  RsaSignatureSecretKey(RsaSkCtx Ctx) : Ctx(std::move(Ctx)) {}

  static WasiCryptoExpect<SignatureSecretKey>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_secretkey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_secretkey_encoding_e_t Encoding) override;

private:
  RsaSkCtx Ctx;
};

class RsaSignatureKeyPair : public SignatureKeyPair::Base {
public:
  RsaSignatureKeyPair(RsaKpCtx Ctx) : Ctx(std::move(Ctx)) {}

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
  RsaKpCtx Ctx;
};

class RsaSignature : public Signature::Base {
public:
  RsaSignature(RsaSignCtx Ctx) : Ctx(std::move(Ctx)) {}

  static WasiCryptoExpect<Signature>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_signature_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_signature_encoding_e_t Encoding) override;

  Span<uint8_t const> asRef() override { return Ctx.asRef(); }

private:
  RsaSignCtx Ctx;
};

class RsaSignatureState : public SignatureState::Base {
public:
  RsaSignatureState(RsaSignStateCtx Ctx) : Ctx(std::move(Ctx)) {}

  WasiCryptoExpect<void> update(Span<uint8_t const> Input) override;

  WasiCryptoExpect<Signature> sign() override;

private:
  RsaSignStateCtx Ctx;
};

class RsaSignatureVerificationState : public SignatureVerificationState::Base {
public:
  RsaSignatureVerificationState(RsaVerificationCtx Ctx) : Ctx(std::move(Ctx)){};

  WasiCryptoExpect<void> update(Span<const uint8_t> Input) override;

  WasiCryptoExpect<void> verify(std::unique_ptr<Signature::Base> &Sig) override;

private:
  RsaVerificationCtx Ctx;
};

class RsaSignStateCtx;
class RsaVerificationCtx;

class RsaPkCtx {
public:
  RsaPkCtx(OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Pk) : Pk(std::move(Pk)) {}

  static WasiCryptoExpect<RsaPkCtx>
  import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
         __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<RsaVerificationCtx> asVerification();

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Pk;
};

class RsaSkCtx {
public:
  RsaSkCtx(OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Sk) : Sk(std::move(Sk)) {}

  static WasiCryptoExpect<RsaSkCtx>
  import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
         __wasi_secretkey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_secretkey_encoding_e_t Encoding);

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Sk;
};

class RsaKpCtx {
public:
  RsaKpCtx(OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Kp) : Kp(std::move(Kp)) {}

  static WasiCryptoExpect<RsaKpCtx>
  import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
         __wasi_keypair_encoding_e_t Encoding);

  static WasiCryptoExpect<RsaKpCtx> generate(SignatureAlgorithm Alg);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<RsaSignStateCtx> asSignState();

  WasiCryptoExpect<RsaPkCtx> publicKey();

  WasiCryptoExpect<RsaSkCtx> secretKey();

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Kp;
};

class RsaSignCtx {
public:
  RsaSignCtx(std::vector<uint8_t> &&Sign) : Sign(std::move(Sign)) {}

  static WasiCryptoExpect<RsaSignCtx>
  import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
         __wasi_signature_encoding_e_t Encoding);

  Span<uint8_t const> asRef() { return Sign; };

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_signature_encoding_e_t Encoding);

private:
  std::vector<uint8_t> Sign;
};

class RsaSignStateCtx {
public:
  RsaSignStateCtx(OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> MdCtx)
      : MdCtx(std::move(MdCtx)) {}

  WasiCryptoExpect<void> update(Span<uint8_t const> Data);

  WasiCryptoExpect<RsaSignCtx> sign();

private:
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> MdCtx;
};

class RsaVerificationCtx {
public:
  RsaVerificationCtx(OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> MdCtx)
      : MdCtx(std::move(MdCtx)) {}

  WasiCryptoExpect<void> update(Span<uint8_t const> Data);

  WasiCryptoExpect<void> verify(Span<uint8_t const> Data);

private:
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> MdCtx;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
