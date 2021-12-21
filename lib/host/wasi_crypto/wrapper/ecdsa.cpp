// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/wrapper/ecdsa.h"
#include "openssl/decoder.h"
#include "openssl/ec.h"
#include "openssl/encoder.h"
#include "openssl/evp.h"
#include "openssl/pem.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<EcdsaPkCtx>
EcdsaPkCtx::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                   __wasi_publickey_encoding_e_t Encoding) {
  EVP_PKEY *Pk = nullptr;

  // Select encoding
  const char *EncodingType;
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    EncodingType = "RAW";
    break;
  }
  case __WASI_PUBLICKEY_ENCODING_PKCS8: {
    EncodingType = "PKCS#8";
    break;
  }
  case __WASI_PUBLICKEY_ENCODING_PEM: {
    EncodingType = "PEM";
    break;
  }
  case __WASI_PUBLICKEY_ENCODING_SEC: {
    EncodingType = "SEC";
    break;
  }
  case __WASI_PUBLICKEY_ENCODING_COMPRESSED_SEC: {
    EncodingType = "SEC";
    break;
  }
  case __WASI_PUBLICKEY_ENCODING_LOCAL: {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    break;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  OpenSSLUniquePtr<OSSL_DECODER_CTX, OSSL_DECODER_CTX_free> DecoderCtx{
      OSSL_DECODER_CTX_new_for_pkey(&Pk, EncodingType, nullptr, "ED",
                                    EVP_PKEY_PUBLIC_KEY, nullptr, nullptr)};
  if (DecoderCtx == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  // fill Pk
  const unsigned char *Data = Encoded.data();
  size_t DataLen = Encoded.size();
  if (1 != OSSL_DECODER_from_data(DecoderCtx.get(), &Data, &DataLen)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return EcdsaPkCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{Pk}};
}

WasiCryptoExpect<std::vector<uint8_t>>
EcdsaPkCtx::exportData(__wasi_publickey_encoding_e_t Encoding) {
  const char *EncodingType;
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    EncodingType = "RAW";
    break;
  }
  case __WASI_PUBLICKEY_ENCODING_PKCS8: {
    EncodingType = "PKCS#8";
    break;
  }
  case __WASI_PUBLICKEY_ENCODING_PEM: {
    EncodingType = "PEM";
    break;
  }
  case __WASI_PUBLICKEY_ENCODING_SEC: {
    EncodingType = "SEC";
    break;
  }
  case __WASI_PUBLICKEY_ENCODING_COMPRESSED_SEC: {
    EncodingType = "SEC";
    break;
  }
  case __WASI_PUBLICKEY_ENCODING_LOCAL: {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    break;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  OpenSSLUniquePtr<OSSL_ENCODER_CTX, OSSL_ENCODER_CTX_free> EncoderCtx{
      OSSL_ENCODER_CTX_new_for_pkey(Pk.get(), EVP_PKEY_PUBLIC_KEY, EncodingType,
                                    nullptr, nullptr)};

  unsigned char *Data = nullptr;
  size_t DataLen;
  if (1 != OSSL_ENCODER_to_data(EncoderCtx.get(), &Data, &DataLen)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  // extra copy
  std::vector<uint8_t> Res;
  Res.assign(Data, Data + DataLen);

  OPENSSL_free(Data);

  return Res;
}

WasiCryptoExpect<EcdsaVerificationCtx> EcdsaPkCtx::asVerification() {
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> SignCtx{EVP_MD_CTX_create()};
  assuming(SignCtx);
  assuming(EVP_DigestVerifyInit(SignCtx.get(), nullptr, EVP_sha256(), nullptr,
                                Pk.get()));

  return EcdsaVerificationCtx{std::move(SignCtx)};
}

WasiCryptoExpect<EcdsaSkCtx>
EcdsaSkCtx::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                   __wasi_secretkey_encoding_e_t Encoding) {
  EVP_PKEY *Sk = nullptr;

  // Select encoding
  const char *EncodingType;
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW: {
    EncodingType = "RAW";
    break;
  }
  case __WASI_SECRETKEY_ENCODING_PKCS8: {
    EncodingType = "PKCS#8";
    break;
  }
  case __WASI_SECRETKEY_ENCODING_PEM: {
    EncodingType = "PEM";
    break;
  }
  case __WASI_SECRETKEY_ENCODING_SEC: {
    EncodingType = "SEC";
    break;
  }
  case __WASI_SECRETKEY_ENCODING_COMPRESSED_SEC: {
    EncodingType = "SEC";
    break;
  }
  case __WASI_SECRETKEY_ENCODING_LOCAL: {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    break;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  OpenSSLUniquePtr<OSSL_DECODER_CTX, OSSL_DECODER_CTX_free> DecoderCtx{
      OSSL_DECODER_CTX_new_for_pkey(&Sk, EncodingType, nullptr, "ED",
                                    OSSL_KEYMGMT_SELECT_PRIVATE_KEY, nullptr,
                                    nullptr)};
  if (DecoderCtx == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  // fill Pk
  const unsigned char *Data = Encoded.data();
  size_t DataLen = Encoded.size();
  if (1 != OSSL_DECODER_from_data(DecoderCtx.get(), &Data, &DataLen)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return EcdsaSkCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{Sk}};
}

WasiCryptoExpect<std::vector<uint8_t>>
EcdsaSkCtx::exportData(__wasi_secretkey_encoding_e_t Encoding) {
  const char *EncodingType;
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW: {
    EncodingType = "RAW";
    break;
  }
  case __WASI_SECRETKEY_ENCODING_PKCS8: {
    EncodingType = "PKCS#8";
    break;
  }
  case __WASI_SECRETKEY_ENCODING_PEM: {
    EncodingType = "PEM";
    break;
  }
  case __WASI_SECRETKEY_ENCODING_SEC: {
    EncodingType = "SEC";
    break;
  }
  case __WASI_SECRETKEY_ENCODING_COMPRESSED_SEC: {
    EncodingType = "SEC";
    break;
  }
  case __WASI_SECRETKEY_ENCODING_LOCAL: {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    break;
  }
  default:
    __builtin_unreachable();
  }

  OpenSSLUniquePtr<OSSL_ENCODER_CTX, OSSL_ENCODER_CTX_free> EncoderCtx{
      OSSL_ENCODER_CTX_new_for_pkey(Sk.get(), OSSL_KEYMGMT_SELECT_PRIVATE_KEY,
                                    EncodingType, nullptr, nullptr)};

  unsigned char *Data = nullptr;
  size_t DataLen;
  if (1 != OSSL_ENCODER_to_data(EncoderCtx.get(), &Data, &DataLen)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  // extra copy
  std::vector<uint8_t> Res;
  Res.assign(Data, Data + DataLen);

  OPENSSL_free(Data);

  return Res;
}

WasiCryptoExpect<EcdsaKpCtx>
EcdsaKpCtx::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                   __wasi_keypair_encoding_e_t Encoding) {
  EVP_PKEY *Kp = nullptr;

  // Select encoding
  const char *EncodingType;
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    EncodingType = "RAW";
    break;
  }
  case __WASI_KEYPAIR_ENCODING_PKCS8: {
    EncodingType = "PKCS#8";
    break;
  }
  case __WASI_KEYPAIR_ENCODING_PEM: {
    EncodingType = "PEM";
    break;
  }
  case __WASI_KEYPAIR_ENCODING_LOCAL: {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    break;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  OpenSSLUniquePtr<OSSL_DECODER_CTX, OSSL_DECODER_CTX_free> DecoderCtx{
      OSSL_DECODER_CTX_new_for_pkey(&Kp, EncodingType, nullptr, "ED",
                                    EVP_PKEY_KEYPAIR, nullptr, nullptr)};
  if (DecoderCtx == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  // fill Pk
  const unsigned char *Data = Encoded.data();
  size_t DataLen = Encoded.size();
  if (1 != OSSL_DECODER_from_data(DecoderCtx.get(), &Data, &DataLen)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return EcdsaKpCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{Kp}};
}

WasiCryptoExpect<EcdsaSignStateCtx> EcdsaKpCtx::asSignState() {
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> SignCtx{EVP_MD_CTX_create()};
  assuming(SignCtx);

  // TODO:It pass a keypair but need a private key. Later to check
  assuming(EVP_DigestSignInit(SignCtx.get(), nullptr, EVP_sha256(), nullptr,
                              Kp.get()));

  return EcdsaSignStateCtx{std::move(SignCtx)};
}

WasiCryptoExpect<EcdsaKpCtx> EcdsaKpCtx::generate(SignatureAlgorithm Alg) {
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx{
      EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr)};

  if (Ctx == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  if (1 != EVP_PKEY_keygen_init(Ctx.get())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  EVP_PKEY *Kp = nullptr;
  if (EVP_PKEY_generate(Ctx.get(), &Kp) != 1) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return EcdsaKpCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{Kp}};
}

WasiCryptoExpect<std::vector<uint8_t>>
EcdsaKpCtx::exportData(__wasi_keypair_encoding_e_t Encoding) {
  const char *EncodingType;
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    EncodingType = "RAW";
    break;
  }
  case __WASI_KEYPAIR_ENCODING_PKCS8: {
    EncodingType = "PKCS#8";
    break;
  }
  case __WASI_KEYPAIR_ENCODING_PEM: {
    EncodingType = "PEM";
    break;
  }
  case __WASI_KEYPAIR_ENCODING_LOCAL: {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    break;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  OpenSSLUniquePtr<OSSL_ENCODER_CTX, OSSL_ENCODER_CTX_free> EncoderCtx{
      OSSL_ENCODER_CTX_new_for_pkey(Kp.get(), EVP_PKEY_KEYPAIR, EncodingType,
                                    nullptr, nullptr)};

  unsigned char *Data = nullptr;
  size_t DataLen;
  if (1 != OSSL_ENCODER_to_data(EncoderCtx.get(), &Data, &DataLen)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  // extra copy
  std::vector<uint8_t> Res;
  Res.assign(Data, Data + DataLen);

  OPENSSL_free(Data);

  return Res;
}

WasiCryptoExpect<EcdsaPkCtx> EcdsaKpCtx::publicKey() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<EcdsaSkCtx> EcdsaKpCtx::secretKey() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<EcdsaSignCtx>
EcdsaSignCtx::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                     __wasi_signature_encoding_e_t Encoding) {

  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW: {
    break;
  }
  case __WASI_SIGNATURE_ENCODING_DER: {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    break;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return EcdsaSignCtx{std::vector<uint8_t>{Encoded.begin(), Encoded.end()}};
}

WasiCryptoExpect<std::vector<uint8_t>>
EcdsaSignCtx::exportData(__wasi_signature_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW: {
    break;
  }
  case __WASI_SIGNATURE_ENCODING_DER: {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    break;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  return Sign;
}

WasiCryptoExpect<void> EcdsaSignStateCtx::update(Span<const uint8_t> Data) {
  assuming(EVP_DigestSignUpdate(MdCtx.get(), Data.data(), Data.size()));

  return {};
}

WasiCryptoExpect<EcdsaSignCtx> EcdsaSignStateCtx::sign() {
  size_t Size;
  assuming(EVP_DigestSignFinal(MdCtx.get(), nullptr, &Size));

  std::vector<uint8_t> Res;
  Res.reserve(Size);
  Res.resize(Size);

  assuming(EVP_DigestSignFinal(MdCtx.get(), Res.data(), &Size));

  return Res;
}

WasiCryptoExpect<void> EcdsaVerificationCtx::update(Span<const uint8_t> Data) {
  assuming(EVP_DigestVerifyUpdate(MdCtx.get(), Data.data(), Data.size()));
  return {};
}

WasiCryptoExpect<void> EcdsaVerificationCtx::verify(Span<const uint8_t> Data) {
  assuming(EVP_DigestVerifyFinal(MdCtx.get(), Data.data(), Data.size()));

  return {};
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
