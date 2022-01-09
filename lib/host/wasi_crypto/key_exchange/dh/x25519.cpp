// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/dh/x25519.h"
#include <openssl/x509.h>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Kx {

WasiCryptoExpect<std::unique_ptr<X25519PublicKey>>
X25519PublicKey::import(Span<const uint8_t> Encoded,
                        __wasi_publickey_encoding_e_t Encoding) {
  EVP_PKEY *Pk = nullptr;
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    const uint8_t *Temp = Encoded.data();
    Pk = d2i_PublicKey(EVP_PKEY_X25519, &Pk, &Temp, Encoded.size());
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
    assumingUnreachable();
  }

  return std::make_unique<X25519PublicKey>(Pk);
}

WasiCryptoExpect<std::vector<uint8_t>> X25519PublicKey::exportData(__wasi_publickey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW:{
    std::vector<uint8_t> Res(i2d_PublicKey(Pk.get(), nullptr));
    uint8_t *Temp = Res.data();
    opensslAssuming(i2d_PublicKey(Pk.get(), &Temp));
    return Res;
  }
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
  default:
    assumingUnreachable();
  }
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<void> X25519PublicKey::verify() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::unique_ptr<X25519SecretKey>>
X25519SecretKey::import(Span<const uint8_t> Encoded,
                        __wasi_secretkey_encoding_e_t Encoding) {
  EVP_PKEY *Sk = nullptr;
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW: {
    const uint8_t *Temp = Encoded.data();
    Sk = d2i_PrivateKey(EVP_PKEY_X25519, &Sk, &Temp, Encoded.size());
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
    assumingUnreachable();
  }
  return std::make_unique<X25519SecretKey>(Sk);
}

WasiCryptoExpect<std::vector<uint8_t>>
X25519SecretKey::exportData(__wasi_secretkey_encoding_e_t Encoding) {
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
    assumingUnreachable();
  }
}

WasiCryptoExpect<std::unique_ptr<PublicKey>> X25519SecretKey::publicKey() {
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> PCtx{
      EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr)};
  opensslAssuming(PCtx);
  opensslAssuming(EVP_PKEY_keygen_init(PCtx.get()));

  EVP_PKEY *P = nullptr;
  opensslAssuming(EVP_PKEY_keygen(PCtx.get(), &P));

  return std::make_unique<X25519PublicKey>(P);
}

WasiCryptoExpect<std::vector<uint8_t>>
X25519SecretKey::dh(std::shared_ptr<PublicKey> Pk) {
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx{
      EVP_PKEY_CTX_new(Sk.get(), nullptr)};
  opensslAssuming(Ctx);
  opensslAssuming(EVP_PKEY_derive_init(Ctx.get()));

  // normally peer will be a public key
  opensslAssuming(EVP_PKEY_derive_set_peer(Ctx.get(), Pk.Pk.get()));

  size_t Size;
  opensslAssuming(EVP_PKEY_derive(Ctx.get(), nullptr, &Size));

  std::vector<uint8_t> Res;
  Res.reserve(Size);
  Res.resize(Size);
  opensslAssuming(EVP_PKEY_derive(Ctx.get(), Res.data(), &Size));

  return Res;
}

WasiCryptoExpect<std::unique_ptr<KeyPair>>
X25519KeyPair::Builder::generate(std::optional<Options> /*TODO:Options*/) {
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ct{
      EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr)};
  opensslAssuming(Ct);
  opensslAssuming(EVP_PKEY_keygen_init(Ct.get()));

  EVP_PKEY *PKey = nullptr;
  opensslAssuming(EVP_PKEY_keygen(Ct.get(), &PKey));

  return std::make_unique<X25519KeyPair>(PKey);
}

WasiCryptoExpect<std::unique_ptr<KeyPair>>
X25519KeyPair::Builder::import(Span<const uint8_t>,
                               __wasi_keypair_encoding_e_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<void> X25519KeyPair::verify() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::unique_ptr<PublicKey>> X25519KeyPair::publicKey() {
  OpenSSLUniquePtr<BIO, BIO_free> B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PUBKEY_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PUBKEY_bio(B.get(), &Res));

  return std::make_unique<X25519PublicKey>(Res);
}

WasiCryptoExpect<std::unique_ptr<SecretKey>> X25519KeyPair::secretKey() {
  OpenSSLUniquePtr<BIO, BIO_free> B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PrivateKey_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PrivateKey_bio(B.get(), &Res));

  return std::make_unique<X25519SecretKey>(Res);
}

WasiCryptoExpect<std::vector<uint8_t>>
X25519KeyPair::exportData(__wasi_keypair_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    std::vector<uint8_t> Res(i2d_PrivateKey(Ctx.get(), nullptr));
    uint8_t *Temp = Res.data();
    opensslAssuming(i2d_PrivateKey(Ctx.get(), &Temp));
    return Res;
  }
  case __WASI_KEYPAIR_ENCODING_PKCS8:
    break;
  case __WASI_KEYPAIR_ENCODING_PEM:
    break;
  case __WASI_KEYPAIR_ENCODING_LOCAL:
    break;
  default:
    assumingUnreachable();
  }
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

} // namespace Kx
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
