// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/wrapper/ecdsa.h"
#include <openssl/ec.h>
#include <openssl/obj_mac.h>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<EcdsaPkCtx>
EcdsaPkCtx::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                   __wasi_publickey_encoding_e_t Encoding) {
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Pk{EVP_PKEY_new_raw_public_key(
      EVP_PKEY_EC, nullptr, Encoded.data(), Encoded.size())};

  if (Pk == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW:
    break;
  case __WASI_PUBLICKEY_ENCODING_PKCS8:
    break;
  case __WASI_PUBLICKEY_ENCODING_PEM:
    break;
  case __WASI_PUBLICKEY_ENCODING_SEC:
    break;
  case __WASI_PUBLICKEY_ENCODING_COMPRESSED_SEC:
    break;
  case __WASI_PUBLICKEY_ENCODING_LOCAL:
    break;
  }

  return EcdsaPkCtx{std::move(Pk)};
}

WasiCryptoExpect<std::vector<uint8_t>>
EcdsaPkCtx::exportData(__wasi_publickey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW:
    break;
  case __WASI_PUBLICKEY_ENCODING_PKCS8:
    break;
  case __WASI_PUBLICKEY_ENCODING_PEM:
    break;
  case __WASI_PUBLICKEY_ENCODING_SEC:
    break;
  case __WASI_PUBLICKEY_ENCODING_COMPRESSED_SEC:
    break;
  case __WASI_PUBLICKEY_ENCODING_LOCAL:
    break;
  }

  size_t ExpectedSize;
  if (1 != EVP_PKEY_get_raw_public_key(Pk.get(), nullptr, &ExpectedSize)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  std::vector<uint8_t> Data;
  Data.reserve(ExpectedSize);
  if (1 != EVP_PKEY_get_raw_public_key(Pk.get(), Data.data(), &ExpectedSize)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return Data;
}

WasiCryptoExpect<EcdsaVerificationCtx> EcdsaPkCtx::asVerification() {
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> MdCtx{EVP_MD_CTX_create()};
  if (MdCtx == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  EVP_PKEY_Q_keygen(nullptr, nullptr, "EC", "P-256");
  // TODO:It pass a keypair but need a private key. Later to check
  if (1 != EVP_DigestVerifyInit(MdCtx.get(), nullptr, EVP_sha256(), nullptr,
                                Pk.get())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return EcdsaVerificationCtx{std::move(MdCtx)};
}

WasiCryptoExpect<EcdsaSkCtx>
EcdsaSkCtx::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                   __wasi_secretkey_encoding_e_t Encoding) {
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Sk{EVP_PKEY_new_raw_private_key(
      EVP_PKEY_EC, nullptr, Encoded.data(), Encoded.size())};
  if (Sk == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return EcdsaSkCtx{std::move(Sk)};
}

WasiCryptoExpect<std::vector<uint8_t>>
EcdsaSkCtx::exportData(__wasi_secretkey_encoding_e_t Encoding) {
  int Size = i2d_PrivateKey(Sk.get(), nullptr);
  std::vector<uint8_t> Res;
  Res.reserve(Size);
  if (auto *Address = Res.data(); i2d_PrivateKey(Sk.get(), &Address) != 1) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  return Res;

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
    break;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
}

WasiCryptoExpect<EcdsaKpCtx> EcdsaKpCtx::import(SignatureAlgorithm,
                                                Span<const uint8_t>,
                                                __wasi_keypair_encoding_e_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  //  switch (Encoding) {
  //  case __WASI_KEYPAIR_ENCODING_RAW: {
  //
  //    return
  //  } break;
  //  case __WASI_KEYPAIR_ENCODING_PKCS8:
  //    break;
  //  case __WASI_KEYPAIR_ENCODING_PEM:
  //    break;
  //  case __WASI_KEYPAIR_ENCODING_LOCAL:
  //    break;
  //  }
}

WasiCryptoExpect<EcdsaSignCtx> EcdsaKpCtx::asSign() {
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> MdCtx{EVP_MD_CTX_create()};
  if (MdCtx == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  switch (Alg) {
  case SignatureAlgorithm::ECDSA_P256_SHA256:
    break;
  case SignatureAlgorithm::ECDSA_K256_SHA256:
    break;
  default:
    break;
  }

  // TODO:It pass a keypair but need a private key. Later to check
  if (1 != EVP_DigestSignInit(MdCtx.get(), nullptr, EVP_sha256(), nullptr,
                              Kp.get())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return EcdsaSignCtx{std::move(MdCtx)};
}

WasiCryptoExpect<EcdsaKpCtx> EcdsaKpCtx::generate(SignatureAlgorithm Alg) {

  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Kp;
  //  if (Kp == nullptr) {
  //    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  //  }

  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx{
      EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr)};
  if (Ctx == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  if (1 != EVP_PKEY_keygen_init(Ctx.get())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  if (auto *Address = Kp.get(); EVP_PKEY_generate(Ctx.get(), &Address) != 1) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return EcdsaKpCtx{std::move(Kp)};
}

WasiCryptoExpect<std::vector<uint8_t>>
EcdsaKpCtx::exportData(__wasi_keypair_encoding_e_t Encoding) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<EcdsaPkCtx> EcdsaKpCtx::publicKey() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<EcdsaSkCtx> EcdsaKpCtx::secretKey() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<void> EcdsaSignCtx::update(Span<const uint8_t> Data) {
  if (1 != EVP_DigestSignUpdate(MdCtx.get(), Data.data(), Data.size())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return {};
}

WasiCryptoExpect<std::vector<uint8_t>> EcdsaSignCtx::sign() {
  size_t Size;
  if (1 != EVP_DigestSignFinal(MdCtx.get(), nullptr, &Size)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  std::vector<uint8_t> Res;
  Res.reserve(Size);
  if (1 != EVP_DigestSignFinal(MdCtx.get(), Res.data(), &Size)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return Res;
}

WasiCryptoExpect<void> EcdsaVerificationCtx::update(Span<const uint8_t> Data) {
  if (1 != EVP_DigestVerifyUpdate(MdCtx.get(), Data.data(), Data.size())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  return {};
}

WasiCryptoExpect<void> EcdsaVerificationCtx::verify(Span<const uint8_t> Data) {
  if (1 != EVP_DigestVerifyFinal(MdCtx.get(), Data.data(), Data.size())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return {};
}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
