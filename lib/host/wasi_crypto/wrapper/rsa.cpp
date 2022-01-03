// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/wrapper/rsa.h"

#include "openssl/evp.h"
#include "openssl/pem.h"
#include "openssl/rsa.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace {
using NID = int;

EVP_PKEY *initRsa(SignatureAlgorithm) {
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> PCtx{
      EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr)};
  opensslAssuming(PCtx);
  EVP_PKEY *ED = nullptr;
  opensslAssuming(EVP_PKEY_paramgen(PCtx.get(), &ED));
  return ED;
}

} // namespace

// raw secret scalar encoded as big endian, SEC-1, compressed SEC-1, unencrypted
// PKCS#8, PEM-encoded unencrypted PKCS#8
WasiCryptoExpect<RsaPkCtx>
RsaPkCtx::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                 __wasi_publickey_encoding_e_t Encoding) {
  EVP_PKEY *Pk = initRsa(Alg);

  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    const uint8_t *Temp = Encoded.data();
    Pk = d2i_PublicKey(EVP_PKEY_RSA, &Pk, &Temp, Encoded.size());
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

  return RsaPkCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{Pk}};
}

WasiCryptoExpect<std::vector<uint8_t>>
RsaPkCtx::exportData(__wasi_publickey_encoding_e_t Encoding) {
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

WasiCryptoExpect<RsaVerificationCtx> RsaPkCtx::asVerification() {
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> SignCtx{EVP_MD_CTX_create()};
  opensslAssuming(SignCtx);
  opensslAssuming(EVP_DigestVerifyInit(SignCtx.get(), nullptr, EVP_sha256(),
                                       nullptr, Pk.get()));

  return RsaVerificationCtx{std::move(SignCtx)};
}

WasiCryptoExpect<RsaSkCtx>
RsaSkCtx::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                 __wasi_secretkey_encoding_e_t Encoding) {
  EVP_PKEY *Sk = initRsa(Alg);

  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW: {
    const uint8_t *Temp = Encoded.data();
    Sk = d2i_PrivateKey(EVP_PKEY_RSA, &Sk, &Temp, Encoded.size());
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

  return RsaSkCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{Sk}};
}

WasiCryptoExpect<std::vector<uint8_t>>
RsaSkCtx::exportData(__wasi_secretkey_encoding_e_t Encoding) {
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

WasiCryptoExpect<RsaKpCtx>
RsaKpCtx::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                 __wasi_keypair_encoding_e_t Encoding) {
  EVP_PKEY *Kp = initRsa(Alg);

  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    const uint8_t *Temp = Encoded.data();
    Kp = d2i_PrivateKey(EVP_PKEY_RSA, &Kp, &Temp, Encoded.size());
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

  return RsaKpCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{Kp}};
}

WasiCryptoExpect<RsaSignStateCtx> RsaKpCtx::asSignState() {
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> SignCtx{EVP_MD_CTX_create()};
  opensslAssuming(SignCtx);

  opensslAssuming(EVP_DigestSignInit(SignCtx.get(), nullptr, EVP_sha256(),
                                     nullptr, Kp.get()));

  return RsaSignStateCtx{std::move(SignCtx)};
}

WasiCryptoExpect<RsaKpCtx> RsaKpCtx::generate(SignatureAlgorithm ) {
  EVP_PKEY_CTX *Ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
  opensslAssuming(Ctx);

  opensslAssuming(EVP_PKEY_keygen_init(Ctx));
//      if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0)
        /* Generate key */
  EVP_PKEY *PKey = nullptr;
  opensslAssuming(EVP_PKEY_keygen(Ctx, &PKey));
//  EVP_PKEY *Params = initRsa(Alg);

  // Generate Key
//  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> KCtx{
//      EVP_PKEY_CTX_new(Params, nullptr)};
//  opensslAssuming(Ctx);
//  opensslAssuming(EVP_PKEY_keygen_init(KCtx.get()));

//  EVP_PKEY *Key = nullptr;
//  opensslAssuming(EVP_PKEY_keygen(KCtx.get(), &Key));

  return RsaKpCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{PKey}};
}

WasiCryptoExpect<std::vector<uint8_t>>
RsaKpCtx::exportData(__wasi_keypair_encoding_e_t Encoding) {
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

WasiCryptoExpect<RsaPkCtx> RsaKpCtx::publicKey() {
  OpenSSLUniquePtr<BIO, BIO_free> B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PUBKEY_bio(B.get(), Kp.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PUBKEY_bio(B.get(), &Res));

  return RsaPkCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>(Res)};
}

WasiCryptoExpect<RsaSkCtx> RsaKpCtx::secretKey() {
  OpenSSLUniquePtr<BIO, BIO_free> B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PrivateKey_bio(B.get(), Kp.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PrivateKey_bio(B.get(), &Res));

  return RsaSkCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{Res}};
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
  opensslAssuming(EVP_DigestSignUpdate(MdCtx.get(), Data.data(), Data.size()));
  return {};
}

WasiCryptoExpect<RsaSignCtx> RsaSignStateCtx::sign() {
  size_t Size;
  opensslAssuming(EVP_DigestSignFinal(MdCtx.get(), nullptr, &Size));

  std::vector<uint8_t> Res;
  Res.reserve(Size);
  Res.resize(Size);

  opensslAssuming(EVP_DigestSignFinal(MdCtx.get(), Res.data(), &Size));

  return Res;
}

WasiCryptoExpect<void> RsaVerificationCtx::update(Span<const uint8_t> Data) {
  opensslAssuming(
      EVP_DigestVerifyUpdate(MdCtx.get(), Data.data(), Data.size()));
  return {};
}

WasiCryptoExpect<void> RsaVerificationCtx::verify(Span<const uint8_t> Data) {
  opensslAssuming(EVP_DigestVerifyFinal(MdCtx.get(), Data.data(), Data.size()));

  return {};
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
