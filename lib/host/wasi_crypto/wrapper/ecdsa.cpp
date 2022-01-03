// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/wrapper/ecdsa.h"
#include "openssl/ec.h"
#include "openssl/evp.h"
#include "openssl/pem.h"
#include <map>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace {
using NID = int;

// TODO:Raw meaning compressed_sec not sec, please check

const std::map<SignatureAlgorithm, NID> AlgToNID{
    {SignatureAlgorithm::ECDSA_P256_SHA256, NID_X9_62_prime256v1},
    {SignatureAlgorithm::ECDSA_K256_SHA256, NID_secp256k1}};

EVP_PKEY *initEC(SignatureAlgorithm Alg) {
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> PCtx{
      EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr)};
  opensslAssuming(PCtx);
  opensslAssuming(EVP_PKEY_paramgen_init(PCtx.get()));
  opensslAssuming(
      EVP_PKEY_CTX_set_ec_paramgen_curve_nid(PCtx.get(), AlgToNID.at(Alg)));

  EVP_PKEY *Params = nullptr;
  opensslAssuming(EVP_PKEY_paramgen(PCtx.get(), &Params));
  return Params;
}

} // namespace

// raw secret scalar encoded as big endian, SEC-1, compressed SEC-1, unencrypted
// PKCS#8, PEM-encoded unencrypted PKCS#8
WasiCryptoExpect<EcdsaPkCtx>
EcdsaPkCtx::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                   __wasi_publickey_encoding_e_t Encoding) {
  EVP_PKEY *Pk = initEC(Alg);
  opensslAssuming(Pk);
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    const uint8_t *Temp = Encoded.data();
    Pk = d2i_PublicKey(EVP_PKEY_EC, &Pk, &Temp, Encoded.size());
    opensslAssuming(Pk);
    break;
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

  return EcdsaPkCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{Pk}};
}

WasiCryptoExpect<std::vector<uint8_t>>
EcdsaPkCtx::exportData(__wasi_publickey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    std::vector<uint8_t> Res(i2d_PublicKey(Pk.get(), nullptr));
    uint8_t *Temp = Res.data();
    opensslAssuming(i2d_PublicKey(Pk.get(), &Temp));
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
  EVP_PKEY * Sk = initEC(Alg);
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW: {
    const uint8_t *Temp = Encoded.data();
    Sk = d2i_PrivateKey(EVP_PKEY_EC, &Sk, &Temp, Encoded.size());
    opensslAssuming(Sk);
    break;
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

  return EcdsaSkCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{Sk}};
}

WasiCryptoExpect<std::vector<uint8_t>>
EcdsaSkCtx::exportData(__wasi_secretkey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW: {
    std::vector<uint8_t> Res(i2d_PrivateKey(Sk.get(), nullptr));
    uint8_t *Temp = Res.data();
    opensslAssuming(i2d_PrivateKey(Sk.get(), &Temp));
    return Res;
  }
  case __WASI_SECRETKEY_ENCODING_PKCS8: {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  }
  case __WASI_SECRETKEY_ENCODING_PEM: {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  }
  case __WASI_SECRETKEY_ENCODING_SEC: {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  }
  case __WASI_SECRETKEY_ENCODING_COMPRESSED_SEC: {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  }
  case __WASI_SECRETKEY_ENCODING_LOCAL: {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  }
  default:
    __builtin_unreachable();
  }
}

WasiCryptoExpect<EcdsaKpCtx>
EcdsaKpCtx::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                   __wasi_keypair_encoding_e_t Encoding) {
  EVP_PKEY *Kp = initEC(Alg);
  opensslAssuming(Kp);
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    const uint8_t *Temp = Encoded.data();
    Kp = d2i_PrivateKey(EVP_PKEY_EC, &Kp, &Temp, Encoded.size());
    opensslAssuming(Kp);
    break;
  }
  case __WASI_KEYPAIR_ENCODING_PKCS8:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  case __WASI_KEYPAIR_ENCODING_PEM:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  case __WASI_KEYPAIR_ENCODING_LOCAL:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  default:
    __builtin_unreachable();
  }

  return EcdsaKpCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{Kp}};
}

WasiCryptoExpect<EcdsaSignStateCtx> EcdsaKpCtx::asSignState() {
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> SignCtx{EVP_MD_CTX_create()};
  opensslAssuming(SignCtx);

  opensslAssuming(EVP_DigestSignInit(SignCtx.get(), nullptr, EVP_sha256(),
                                     nullptr, Kp.get()));

  return EcdsaSignStateCtx{std::move(SignCtx)};
}

WasiCryptoExpect<EcdsaKpCtx> EcdsaKpCtx::generate(SignatureAlgorithm Alg) {

  EVP_PKEY *Params = initEC(Alg);

  // Generate Key
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> KCtx{
      EVP_PKEY_CTX_new(Params, nullptr)};
  opensslAssuming(KCtx);
  opensslAssuming(EVP_PKEY_keygen_init(KCtx.get()));

  EVP_PKEY *Key = nullptr;
  opensslAssuming(EVP_PKEY_keygen(KCtx.get(), &Key));

  return EcdsaKpCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{Key}};
}

WasiCryptoExpect<std::vector<uint8_t>>
EcdsaKpCtx::exportData(__wasi_keypair_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    std::vector<uint8_t> Res(i2d_PrivateKey(Kp.get(), nullptr));
    uint8_t *Temp = Res.data();
    opensslAssuming(i2d_PrivateKey(Kp.get(), &Temp));
    return Res;
  }
  case __WASI_KEYPAIR_ENCODING_PKCS8:
    break;
  case __WASI_KEYPAIR_ENCODING_PEM:
    break;
  case __WASI_KEYPAIR_ENCODING_LOCAL:
    break;
  default:
    __builtin_unreachable();
  }
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<EcdsaPkCtx> EcdsaKpCtx::publicKey() {
  OpenSSLUniquePtr<BIO, BIO_free> B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PUBKEY_bio(B.get(), Kp.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PUBKEY_bio(B.get(), &Res));

  return EcdsaPkCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>(Res)};
}

WasiCryptoExpect<EcdsaSkCtx> EcdsaKpCtx::secretKey() {
  OpenSSLUniquePtr<BIO, BIO_free> B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PrivateKey_bio(B.get(), Kp.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PrivateKey_bio(B.get(), &Res));

  return EcdsaSkCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{Res}};
}

WasiCryptoExpect<EcdsaSignCtx>
EcdsaSignCtx::import(SignatureAlgorithm, Span<const uint8_t>,
                     __wasi_signature_encoding_e_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::vector<uint8_t>>
EcdsaSignCtx::exportData(__wasi_signature_encoding_e_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
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
