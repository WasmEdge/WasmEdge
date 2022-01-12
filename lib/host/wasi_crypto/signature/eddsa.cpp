// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/eddsa.h"

#include "openssl/x509.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

WasiCryptoExpect<std::unique_ptr<EddsaPublicKey>>
EddsaPublicKey::import(Span<const uint8_t> Encoded,
                       __wasi_publickey_encoding_e_t Encoding) {
  EVP_PKEY *Pk = nullptr;
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    const uint8_t *Temp = Encoded.data();
    Pk = d2i_PublicKey(EVP_PKEY_ED25519, &Pk, &Temp, Encoded.size());
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

  return std::make_unique<EddsaPublicKey>(Pk);
}

WasiCryptoExpect<std::vector<uint8_t>>
EddsaPublicKey::exportData(__wasi_publickey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    std::vector<uint8_t> Res(i2d_PublicKey(Ctx.get(), nullptr));
    uint8_t *Temp = Res.data();
    opensslAssuming(i2d_PublicKey(Ctx.get(), &Temp));
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
    assumingUnreachable();
  }
}

WasiCryptoExpect<std::unique_ptr<VerificationState>>
EddsaPublicKey::openVerificationState() {
  EVP_MD_CTX *SignCtx = EVP_MD_CTX_create();
  opensslAssuming(SignCtx);

  opensslAssuming(
      EVP_DigestVerifyInit(SignCtx, nullptr, nullptr, nullptr, Ctx.get()));
  return std::make_unique<EddsaVerificationState>(SignCtx);
}

WasiCryptoExpect<std::unique_ptr<SecretKey>>
EddsaSecretKey::import(Span<const uint8_t> Encoded,
                       __wasi_secretkey_encoding_e_t Encoding) {
  EVP_PKEY *Sk = nullptr;
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW: {
    const uint8_t *Temp = Encoded.data();
    Sk = d2i_PrivateKey(EVP_PKEY_ED25519, &Sk, &Temp, Encoded.size());
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

  return std::make_unique<EddsaSecretKey>(Sk);
}

WasiCryptoExpect<std::vector<uint8_t>>
EddsaSecretKey::exportData(__wasi_secretkey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW: {
    std::vector<uint8_t> Res(i2d_PrivateKey(Ctx.get(), nullptr));
    uint8_t *Temp = Res.data();
    opensslAssuming(i2d_PrivateKey(Ctx.get(), &Temp));
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

WasiCryptoExpect<std::unique_ptr<KeyPair>>
EddsaKeyPair::generate(std::shared_ptr<Options>) {
  // Generate Key
  EvpPkeyCtxPtr KCtx{EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr)};
  opensslAssuming(KCtx);
  opensslAssuming(EVP_PKEY_keygen_init(KCtx.get()));

  EVP_PKEY *Key = nullptr;
  opensslAssuming(EVP_PKEY_keygen(KCtx.get(), &Key));

  return std::make_unique<EddsaKeyPair>(Key);
}

WasiCryptoExpect<std::unique_ptr<KeyPair>>
EddsaKeyPair::import(Span<const uint8_t> Encoded,
                     __wasi_keypair_encoding_e_t Encoding) {
  EVP_PKEY *Kp = nullptr;
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    const uint8_t *Temp = Encoded.data();
    Kp = d2i_PrivateKey(EVP_PKEY_ED25519, &Kp, &Temp, Encoded.size());
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
    assumingUnreachable();
  }

  return std::make_unique<EddsaKeyPair>(Kp);
}

WasiCryptoExpect<std::vector<uint8_t>>
EddsaKeyPair::exportData(__wasi_keypair_encoding_e_t Encoding) {
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

WasiCryptoExpect<std::unique_ptr<PublicKey>> EddsaKeyPair::publicKey() {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PUBKEY_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PUBKEY_bio(B.get(), &Res));

  return std::make_unique<EddsaPublicKey>(Res);
}

WasiCryptoExpect<std::unique_ptr<SecretKey>> EddsaKeyPair::secretKey() {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PrivateKey_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PrivateKey_bio(B.get(), &Res));

  return std::make_unique<EddsaSecretKey>(Res);
}

WasiCryptoExpect<std::unique_ptr<SignState>> EddsaKeyPair::openSignState() {
  EVP_MD_CTX *SignCtx = EVP_MD_CTX_create();
  opensslAssuming(SignCtx);

  opensslAssuming(
      EVP_DigestSignInit(SignCtx, nullptr, nullptr, nullptr, Ctx.get()));

  return std::make_unique<EddsaSignState>(SignCtx);
}

WasiCryptoExpect<std::unique_ptr<Signature>>
EddsaSignature::import(Span<const uint8_t> Encoded,
                       __wasi_signature_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW:
    return std::make_unique<EddsaSignature>(
        std::vector<uint8_t>{Encoded.begin(), Encoded.end()});
  case __WASI_SIGNATURE_ENCODING_DER:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  default:
    assumingUnreachable();
  }
}

WasiCryptoExpect<std::vector<uint8_t>>
EddsaSignature::exportData(__wasi_signature_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW:
    return Data;
  case __WASI_SIGNATURE_ENCODING_DER:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  default:
    assumingUnreachable();
  }
}

WasiCryptoExpect<void> EddsaSignState::update(Span<const uint8_t> Input) {
  // Notice: Ecdsa is oneshot in OpenSSL, we need a cache for update instead of
  // call `EVP_DigestSignUpdate`
  std::unique_lock Lock{Mutex};
  Cache.insert(Cache.end(), Input.begin(), Input.end());
  return {};
}

WasiCryptoExpect<std::unique_ptr<Signature>> EddsaSignState::sign() {
  std::shared_lock Lock{Mutex};
  size_t Size;
  opensslAssuming(
      EVP_DigestSign(Ctx.get(), nullptr, &Size, Cache.data(), Cache.size()));

  std::vector<uint8_t> Res(Size);
  opensslAssuming(
      EVP_DigestSign(Ctx.get(), Res.data(), &Size, Cache.data(), Cache.size()));

  return std::make_unique<EddsaSignature>(std::move(Res));
}

WasiCryptoExpect<void>
EddsaVerificationState::update(Span<const uint8_t> Input) {
  std::unique_lock Lock{Mutex};
  Cache.insert(Cache.end(), Input.begin(), Input.end());
  return {};
}

WasiCryptoExpect<void>
EddsaVerificationState::verify(std::shared_ptr<Signature> Sig) {
  std::shared_lock Lock{Mutex};

  auto Data = Sig->exportData(__WASI_SIGNATURE_ENCODING_RAW);
  if (!Data) {
    return WasiCryptoUnexpect(Data);
  }

  opensslAssuming(EVP_DigestVerify(Ctx.get(), Data->data(), Data->size(),
                                   Cache.data(), Cache.size()));

  return {};
}

} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
