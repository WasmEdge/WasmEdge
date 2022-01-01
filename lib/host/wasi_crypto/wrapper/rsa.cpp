// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/wrapper/rsa.h"
#include "openssl/ec.h"
#include "openssl/evp.h"
#include "openssl/pem.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<RsaPkCtx> RsaPkCtx::import(SignatureAlgorithm,
                                            Span<const uint8_t>,
                                            __wasi_publickey_encoding_e_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::vector<uint8_t>>
RsaPkCtx::exportData(__wasi_publickey_encoding_e_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<RsaVerificationCtx> RsaPkCtx::asVerification() {
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> SignCtx{EVP_MD_CTX_create()};

  if (1 != EVP_DigestVerifyInit(SignCtx.get(), nullptr, EVP_sha256(), nullptr,
                                Pk.get())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return RsaVerificationCtx{std::move(SignCtx)};
}

WasiCryptoExpect<RsaSkCtx> RsaSkCtx::import(SignatureAlgorithm,
                                            Span<const uint8_t>,
                                            __wasi_secretkey_encoding_e_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::vector<uint8_t>>
RsaSkCtx::exportData(__wasi_secretkey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW: {
    break;
  }
  case __WASI_SECRETKEY_ENCODING_PKCS8: {
    break;
  }
  case __WASI_SECRETKEY_ENCODING_PEM: {
    break;
  }
  case __WASI_SECRETKEY_ENCODING_SEC: {
    break;
  }
  case __WASI_SECRETKEY_ENCODING_COMPRESSED_SEC: {
    break;
  }
  case __WASI_SECRETKEY_ENCODING_LOCAL: {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    break;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  // extra copy
  std::vector<uint8_t> Res;

  return Res;
}

WasiCryptoExpect<RsaKpCtx> RsaKpCtx::import(SignatureAlgorithm,
                                            Span<const uint8_t>,
                                            __wasi_keypair_encoding_e_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<RsaSignStateCtx> RsaKpCtx::asSignState() {
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> SignCtx{EVP_MD_CTX_create()};

  // TODO:It pass a keypair but need a private key. Later to check
  if (1 != EVP_DigestSignInit(SignCtx.get(), nullptr, EVP_sha256(), nullptr,
                              Kp.get())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return RsaSignStateCtx{std::move(SignCtx)};
}

WasiCryptoExpect<RsaKpCtx> RsaKpCtx::generate(SignatureAlgorithm) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::vector<uint8_t>>
RsaKpCtx::exportData(__wasi_keypair_encoding_e_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<RsaPkCtx> RsaKpCtx::publicKey() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<RsaSkCtx> RsaKpCtx::secretKey() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<RsaSignCtx> RsaSignCtx::import(SignatureAlgorithm,
                                                Span<const uint8_t>,
                                                __wasi_signature_encoding_e_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::vector<uint8_t>>
RsaSignCtx::exportData(__wasi_signature_encoding_e_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<void> RsaSignStateCtx::update(Span<const uint8_t> Data) {
  if (1 != EVP_DigestSignUpdate(MdCtx.get(), Data.data(), Data.size())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return {};
}

WasiCryptoExpect<RsaSignCtx> RsaSignStateCtx::sign() {
  size_t Size;
  if (1 != EVP_DigestSignFinal(MdCtx.get(), nullptr, &Size)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  std::vector<uint8_t> Res;
  Res.reserve(Size);
  Res.resize(Size);

  if (1 != EVP_DigestSignFinal(MdCtx.get(), Res.data(), &Size)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return Res;
}

WasiCryptoExpect<void> RsaVerificationCtx::update(Span<const uint8_t> Data) {
  if (1 != EVP_DigestVerifyUpdate(MdCtx.get(), Data.data(), Data.size())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  return {};
}

WasiCryptoExpect<void> RsaVerificationCtx::verify(Span<const uint8_t> Data) {
  if (1 != EVP_DigestVerifyFinal(MdCtx.get(), Data.data(), Data.size())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return {};
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
