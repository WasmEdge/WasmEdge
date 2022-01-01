// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/wrapper/ecdsa.h"
#include "openssl/decoder.h"
#include "openssl/ec.h"
#include "openssl/encoder.h"
#include "openssl/evp.h"
#include <map>
#include <openssl/pem.h>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

using NID = int;

std::map<SignatureAlgorithm, NID> AlgToNID = {
    {SignatureAlgorithm::ECDSA_P256_SHA256, NID_X9_62_prime256v1},
    {SignatureAlgorithm::ECDSA_K256_SHA256, NID_secp256k1}};

// raw secret scalar encoded as big endian, SEC-1, compressed SEC-1, unencrypted
// PKCS#8, PEM-encoded unencrypted PKCS#8
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
    // 1.Pass the EVP_PKEY to EVP_PKEY_get0_EC_KEY() to get an EC_KEY.
    const EC_KEY *Ec = EVP_PKEY_get0_EC_KEY(Pk.get());
    opensslAssuming(Ec);

    // 2.Pass the EC_KEY to EC_KEY_get0_public_key() to get an EC_POINT
    const EC_POINT *EcPoint = EC_KEY_get0_public_key(Ec);
    opensslAssuming(EcPoint);

    // 3.Pass the EC_POINT to EC_POINT_point2oct() to get octets
    size_t Size =
        EC_POINT_point2oct(EC_KEY_get0_group(Ec), EcPoint,
                           POINT_CONVERSION_HYBRID, nullptr, 0, nullptr);
    std::vector<uint8_t> Res;
    Res.reserve(Size);
    Res.resize(Size);

    opensslAssuming(EC_POINT_point2oct(EC_KEY_get0_group(Ec), EcPoint,
                                       POINT_CONVERSION_HYBRID, Res.data(),
                                       Res.size(), nullptr));
    return Res;
  }
  case __WASI_PUBLICKEY_ENCODING_PKCS8:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  case __WASI_PUBLICKEY_ENCODING_PEM:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  case __WASI_PUBLICKEY_ENCODING_SEC:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  case __WASI_PUBLICKEY_ENCODING_COMPRESSED_SEC:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  case __WASI_PUBLICKEY_ENCODING_LOCAL:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  default:
    __builtin_unreachable();
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
  opensslAssuming(SignCtx);
  opensslAssuming(EVP_DigestVerifyInit(SignCtx.get(), nullptr, EVP_sha256(),
                                       nullptr, Pk.get()));

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
    OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Sk{EVP_PKEY_new_raw_private_key(
        EVP_PKEY_EC, nullptr, Raw.data(), Raw.size())};
    opensslAssuming(Sk);
    return EcdsaSkCtx{std::move(Sk)};
  }
  case __WASI_SECRETKEY_ENCODING_PKCS8:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  case __WASI_SECRETKEY_ENCODING_PEM:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  case __WASI_SECRETKEY_ENCODING_SEC:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  case __WASI_SECRETKEY_ENCODING_COMPRESSED_SEC:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  case __WASI_SECRETKEY_ENCODING_LOCAL:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  default:
    __builtin_unreachable();
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
  opensslAssuming(SignCtx);

  // TODO:It pass a keypair but need a private key. Later to check
  opensslAssuming(EVP_DigestSignInit(SignCtx.get(), nullptr, EVP_sha256(),
                                     nullptr, Ctx.get()));

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
  // get original group and public key
  const EC_KEY *ECKey = EVP_PKEY_get0_EC_KEY(Ctx.get());
  const EC_GROUP *Group = EC_KEY_get0_group(ECKey);
  const EC_POINT *PublicKey = EC_KEY_get0_public_key(ECKey);

  // copy group and public key
  EC_GROUP *NewGroup = nullptr;
  opensslAssuming(EC_GROUP_copy(NewGroup, Group));
  EC_POINT *NewPublicKey = nullptr;
  opensslAssuming(EC_POINT_copy(NewPublicKey, PublicKey));

  // set group and public key
  EC_KEY *NewKey = EC_KEY_new();
  opensslAssuming(EC_KEY_set_group(NewKey, NewGroup));
  opensslAssuming(EC_KEY_set_public_key(NewKey, PublicKey));

  // construct EVP_PKEY from EC_KEY
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Key{EVP_PKEY_new()};
  opensslAssuming(Key);
  opensslAssuming(EVP_PKEY_set1_EC_KEY(Key.get(), NewKey));

  return EcdsaPkCtx{std::move(Key)};
}

WasiCryptoExpect<EcdsaSkCtx> EcdsaKpCtx::secretKey() {
  OpenSSLUniquePtr<BIO, BIO_free> B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PrivateKey_bio(B.get(), Ctx.get()));

  EVP_PKEY *K = nullptr;
  opensslAssuming(d2i_PrivateKey_bio(B.get(), &K));

  return EcdsaSkCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{K}};
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
  opensslAssuming(EVP_DigestSignUpdate(MdCtx.get(), Data.data(), Data.size()));

  return {};
}

WasiCryptoExpect<EcdsaSignCtx> EcdsaSignStateCtx::sign() {
  size_t Size;
  opensslAssuming(EVP_DigestSignFinal(MdCtx.get(), nullptr, &Size));

  std::vector<uint8_t> Res;
  Res.reserve(Size);
  Res.resize(Size);

  opensslAssuming(EVP_DigestSignFinal(MdCtx.get(), Res.data(), &Size));

  return Res;
}

WasiCryptoExpect<void> EcdsaVerificationCtx::update(Span<const uint8_t> Data) {
  opensslAssuming(
      EVP_DigestVerifyUpdate(MdCtx.get(), Data.data(), Data.size()));
  return {};
}

WasiCryptoExpect<void> EcdsaVerificationCtx::verify(Span<const uint8_t> Data) {
  opensslAssuming(EVP_DigestVerifyFinal(MdCtx.get(), Data.data(), Data.size()));

  return {};
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
