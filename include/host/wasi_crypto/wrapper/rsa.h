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
class RsaSignStateCtx;
class RsaVerificationCtx;

class RsaPkCtx {
public:
  RsaPkCtx(OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Pk)
      : Pk(std::move(Pk)) {}

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
  RsaSkCtx(OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Sk)
      : Sk(std::move(Sk)) {}

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
  RsaKpCtx(OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Kp)
      : Kp(std::move(Kp)) {}

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
