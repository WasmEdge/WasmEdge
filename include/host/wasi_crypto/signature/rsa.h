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
#include "openssl/evp.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

class RsaPublicKey : public PublicKey {
public:
  RsaPublicKey(EVP_PKEY *Pk) : Pk(Pk) {}

  static WasiCryptoExpect<std::unique_ptr<PublicKey>>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_publickey_encoding_e_t Encoding) override;

  WasiCryptoExpect<std::unique_ptr<VerificationState>>
  openVerificationState() override;

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Pk;
};

class RsaSecretKey : public SecretKey {
public:
  RsaSecretKey(EVP_PKEY *Sk) : Sk(std::move(Sk)) {}

  static WasiCryptoExpect<std::unique_ptr<RsaSecretKey>>
  import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
         __wasi_secretkey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_secretkey_encoding_e_t Encoding) override;

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Sk;
};

class RsaKeyPair : public KeyPair {
public:
  RsaKeyPair(EVP_PKEY *Kp) : Kp(std::move(Kp)) {}

  static WasiCryptoExpect<std::unique_ptr<RsaKeyPair>>
  import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
         __wasi_keypair_encoding_e_t Encoding);

  static WasiCryptoExpect<std::unique_ptr<RsaKeyPair>>
  generate(SignatureAlgorithm Alg, std::shared_ptr<Options> OptOptions);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_keypair_encoding_e_t Encoding) override;

  WasiCryptoExpect<std::unique_ptr<SignState>> openSignState() override;

  WasiCryptoExpect<std::unique_ptr<PublicKey>> publicKey() override;

  WasiCryptoExpect<std::unique_ptr<SecretKey>> secretKey() override;

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Kp;
};

class RsaSignature : public Signature {
public:
  RsaSignature(std::vector<uint8_t> &&Sign) : Sign(std::move(Sign)) {}

  static WasiCryptoExpect<std::unique_ptr<RsaSignature>>
  import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
         __wasi_signature_encoding_e_t Encoding);

  Span<uint8_t const> asRef() override { return Sign; };

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_signature_encoding_e_t Encoding) override;

private:
  std::vector<uint8_t> Sign;
};

class RsaSignState : public SignState {
public:
  RsaSignState(EVP_MD_CTX *MdCtx) : MdCtx(MdCtx) {}

  WasiCryptoExpect<void> update(Span<uint8_t const> Data) override;

  WasiCryptoExpect<std::unique_ptr<Signature>> sign() override;

private:
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> MdCtx;
};

class RsaVerificationState : public VerificationState {
public:
  RsaVerificationState(EVP_MD_CTX *MdCtx) : MdCtx(MdCtx) {}

  WasiCryptoExpect<void> update(Span<uint8_t const> Data) override;

  WasiCryptoExpect<void> verify(std::shared_ptr<Signature> Sig) override;

private:
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> MdCtx;
};

} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
