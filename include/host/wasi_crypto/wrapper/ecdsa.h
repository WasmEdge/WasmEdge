// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/signature/alg.h"
#include "host/wasi_crypto/wrapper/openssl.h"
#include "openssl/evp.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
class EcdsaSignCtx;
class EcdsaVerificationCtx;

class EcdsaPkCtx {
public:
  EcdsaPkCtx(OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Pk)
      : Pk(std::move(Pk)) {}

  static WasiCryptoExpect<EcdsaPkCtx>
  import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
         __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<EcdsaVerificationCtx> asVerification();

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Pk;
  SignatureAlgorithm Alg;
};

class EcdsaSkCtx {
public:
  EcdsaSkCtx(OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Sk)
      : Sk(std::move(Sk)) {}

  static WasiCryptoExpect<EcdsaSkCtx>
  import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
         __wasi_secretkey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_secretkey_encoding_e_t Encoding);

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Sk;
  SignatureAlgorithm Alg;
};

class EcdsaKpCtx {
public:
  EcdsaKpCtx(OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Kp)
      : Kp(std::move(Kp)) {}

  static WasiCryptoExpect<EcdsaKpCtx>
  import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
         __wasi_keypair_encoding_e_t Encoding);

  static WasiCryptoExpect<EcdsaKpCtx> generate(SignatureAlgorithm Alg);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<EcdsaSignCtx> asSign();

  WasiCryptoExpect<EcdsaPkCtx> publicKey();

  WasiCryptoExpect<EcdsaSkCtx> secretKey();

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Kp;
  SignatureAlgorithm Alg;
};

class EcdsaSignCtx {
public:
  EcdsaSignCtx(OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> MdCtx)
      : MdCtx(std::move(MdCtx)) {}

  WasiCryptoExpect<void> update(Span<uint8_t const> Data);

  WasiCryptoExpect<std::vector<uint8_t>> sign();

private:
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> MdCtx;
  SignatureAlgorithm Alg;
};

class EcdsaVerificationCtx {
public:
  EcdsaVerificationCtx(OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> MdCtx)
      : MdCtx(std::move(MdCtx)) {}

  WasiCryptoExpect<void> update(Span<uint8_t const> Data);

  WasiCryptoExpect<void> verify(Span<uint8_t const> Data);

private:
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> MdCtx;
  SignatureAlgorithm Alg;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
