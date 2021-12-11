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
class EddsaSignStateCtx;
class EddsaVerificationCtx;

class EddsaPkCtx {
public:
  EddsaPkCtx(OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Pk)
      : Pk(std::move(Pk)) {}

  static WasiCryptoExpect<EddsaPkCtx>
  import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
         __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<EddsaVerificationCtx> asVerification();

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Pk;
};

class EddsaSkCtx {
public:
  EddsaSkCtx(OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Sk)
      : Sk(std::move(Sk)) {}

  static WasiCryptoExpect<EddsaSkCtx>
  import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
         __wasi_secretkey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_secretkey_encoding_e_t Encoding);

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Sk;
};

class EddsaKpCtx {
public:
  EddsaKpCtx(OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Kp)
      : Kp(std::move(Kp)) {}

  static WasiCryptoExpect<EddsaKpCtx>
  import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
         __wasi_keypair_encoding_e_t Encoding);

  static WasiCryptoExpect<EddsaKpCtx> generate(SignatureAlgorithm Alg);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<EddsaSignStateCtx> asSignState();

  WasiCryptoExpect<EddsaPkCtx> publicKey();

  WasiCryptoExpect<EddsaSkCtx> secretKey();

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Kp;
};

class EddsaSignCtx {
public:
  EddsaSignCtx(std::vector<uint8_t> &&Sign) : Sign(std::move(Sign)) {}

  static WasiCryptoExpect<EddsaSignCtx>
  import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
         __wasi_signature_encoding_e_t Encoding);

  Span<uint8_t const> asRef() { return Sign; };

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_signature_encoding_e_t Encoding);

private:
  std::vector<uint8_t> Sign;
};

class EddsaSignStateCtx {
public:
  EddsaSignStateCtx(OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> MdCtx)
      : MdCtx(std::move(MdCtx)) {}

  WasiCryptoExpect<void> update(Span<uint8_t const> Data);

  WasiCryptoExpect<EddsaSignCtx> sign();

private:
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> MdCtx;
};

class EddsaVerificationCtx {
public:
  EddsaVerificationCtx(OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> MdCtx)
      : MdCtx(std::move(MdCtx)) {}

  WasiCryptoExpect<void> update(Span<uint8_t const> Data);

  WasiCryptoExpect<void> verify(Span<uint8_t const> Data);

private:
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> MdCtx;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
