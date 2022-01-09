// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/signature/keypair.h"
#include "host/wasi_crypto/signature/options.h"
#include "host/wasi_crypto/signature/publickey.h"
#include "host/wasi_crypto/signature/secretkey.h"
#include "host/wasi_crypto/signature/signature.h"
#include "openssl/evp.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

class EddsaPublicKey : public PublicKey {
public:
  EddsaPublicKey(EVP_PKEY *Pk) : Ctx(std::move(Pk)) {}

  static WasiCryptoExpect<std::unique_ptr<EddsaPublicKey>>
  import(Span<uint8_t const> Encoded, __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_publickey_encoding_e_t Encoding) override;

  WasiCryptoExpect<std::unique_ptr<VerificationState>>
  openVerificationState() override;

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Ctx;
};

class EddsaSecretKey : public SecretKey {
public:
  EddsaSecretKey(EVP_PKEY *Sk) : Sk(Sk) {}

  static WasiCryptoExpect<std::unique_ptr<SecretKey>>
  import(Span<uint8_t const> Encoded, __wasi_secretkey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_secretkey_encoding_e_t Encoding) override;

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Sk;
};

class EddsaKeyPair : public KeyPair {
public:
  EddsaKeyPair(EVP_PKEY *Kp) : Kp(Kp) {}

  static WasiCryptoExpect<std::unique_ptr<KeyPair>>
  generate(std::shared_ptr<Signatures::Options> Options);

  static WasiCryptoExpect<std::unique_ptr<KeyPair>>
  import(Span<uint8_t const> Encoded, __wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_keypair_encoding_e_t Encoding) override;

  WasiCryptoExpect<std::unique_ptr<PublicKey>> publicKey() override;

  WasiCryptoExpect<std::unique_ptr<SecretKey>> secretKey() override;

  WasiCryptoExpect<std::unique_ptr<SignState>> openSignState() override;

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Kp;
};

class EddsaSignature : public Signature {
public:
  EddsaSignature(std::vector<uint8_t> &&Sign) : Sign(std::move(Sign)) {}

  static WasiCryptoExpect<std::unique_ptr<Signature>>
  import(Span<uint8_t const> Encoded, __wasi_signature_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_signature_encoding_e_t Encoding) override;

  Span<uint8_t const> asRef() override { return Sign; }

private:
  const std::vector<uint8_t> Sign;
};

class EddsaSignState : public SignState {
public:
  EddsaSignState(EVP_MD_CTX *MdCtx) : MdCtx(MdCtx) {}

  WasiCryptoExpect<void> update(Span<uint8_t const> Input) override;

  WasiCryptoExpect<std::unique_ptr<Signature>> sign() override;

private:
  std::vector<uint8_t> Cache;
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> MdCtx;
};

class EddsaVerificationState : public VerificationState {
public:
  EddsaVerificationState(EVP_MD_CTX *MdCtx) : MdCtx(MdCtx) {}

  WasiCryptoExpect<void> update(Span<const uint8_t> Input) override;

  WasiCryptoExpect<void> verify(std::shared_ptr<Signature> Sig) override;

private:
  std::vector<uint8_t> Cache;
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> MdCtx;
};

} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
