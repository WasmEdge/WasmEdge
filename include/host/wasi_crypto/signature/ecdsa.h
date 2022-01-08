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
#include <openssl/evp.h>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

class EcdsaPublicKey : public PublicKey {
public:
  EcdsaPublicKey(EVP_PKEY *Ctx) : Ctx(Ctx) {}

  static WasiCryptoExpect<std::unique_ptr<EcdsaPublicKey>>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_publickey_encoding_e_t Encoding) override;

  WasiCryptoExpect<std::unique_ptr<VerificationState>> asState() override;

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Ctx;
};

class EcdsaSecretKey : public SecretKey {
public:
  EcdsaSecretKey(EVP_PKEY *Ctx) : Ctx(Ctx) {}

  static WasiCryptoExpect<std::unique_ptr<EcdsaSecretKey>>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_secretkey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_secretkey_encoding_e_t Encoding) override;

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Ctx;
};

class EcdsaKeyPair : public KeyPair {
public:
  EcdsaKeyPair(EVP_PKEY *Ctx) : Ctx(std::move(Ctx)) {}

  static WasiCryptoExpect<std::unique_ptr<EcdsaKeyPair>>
  generate(SignatureAlgorithm Alg, std::shared_ptr<Options> Options);

  static WasiCryptoExpect<std::unique_ptr<EcdsaKeyPair>>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_keypair_encoding_e_t Encoding) override;

  WasiCryptoExpect<std::unique_ptr<PublicKey>> publicKey() override;

  WasiCryptoExpect<std::unique_ptr<SecretKey>> secretKey() override;

  WasiCryptoExpect<std::unique_ptr<State>>
  asState(std::shared_ptr<Options> OptOptions) override;

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Ctx;
};

class EcdsaState : public State {
public:
  EcdsaState(EVP_MD_CTX *Ctx) : Ctx(Ctx) {}

  WasiCryptoExpect<void> update(Span<uint8_t const> Input) override;

  WasiCryptoExpect<Signature> sign() override;

private:
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx;
};

class EcdsaVerificationState : public VerificationState {
public:
  EcdsaVerificationState(EVP_MD_CTX *Ctx) : Ctx(Ctx){};

  WasiCryptoExpect<void> update(Span<const uint8_t> Input) override;

  WasiCryptoExpect<void> verify(std::shared_ptr<Signature> Sig) override;

private:
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> Ctx;
};

} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
