// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/eddsa.h"

#include "host/wasi_crypto/error.h"
#include "openssl/x509.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

WasiCryptoExpect<std::unique_ptr<EddsaPublicKey>>
EddsaPublicKey::import(Span<const uint8_t> Encoded,
                       __wasi_publickey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    EVP_PKEY *Pk = nullptr;
    const uint8_t *Temp = Encoded.data();
    ensureOrReturn(Encoded.size() == EddsaPublicKey::Size,
                   __WASI_CRYPTO_ERRNO_INVALID_KEY);

    ensureOrReturn(Encoded.size() <= LONG_MAX,
                   __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
    Pk = d2i_PublicKey(EVP_PKEY_ED25519, &Pk, &Temp,
                       static_cast<long>(Encoded.size()));
    ensureOrReturn(Pk, __WASI_CRYPTO_ERRNO_INVALID_KEY);
    return std::make_unique<EddsaPublicKey>(Pk);
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<std::vector<uint8_t>>
EddsaPublicKey::exportData(__wasi_publickey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    std::vector<uint8_t> Res(
        static_cast<size_t>(i2d_PublicKey(Ctx.get(), nullptr)));
    uint8_t *Temp = Res.data();
    opensslAssuming(i2d_PublicKey(Ctx.get(), &Temp));
    return Res;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
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
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW: {
    EVP_PKEY *Sk = nullptr;
    ensureOrReturn(Encoded.size() == EddsaSecretKey::Size,
                   __WASI_CRYPTO_ERRNO_INVALID_KEY);
    const uint8_t *Temp = Encoded.data();

    ensureOrReturn(Encoded.size() <= LONG_MAX,
                   __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
    Sk = d2i_PrivateKey(EVP_PKEY_ED25519, &Sk, &Temp,
                        static_cast<long>(Encoded.size()));
    ensureOrReturn(Sk, __WASI_CRYPTO_ERRNO_INVALID_KEY);
    return std::make_unique<EddsaSecretKey>(Sk);
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<std::vector<uint8_t>>
EddsaSecretKey::exportData(__wasi_secretkey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW: {
    std::vector<uint8_t> Res(
        static_cast<size_t>(i2d_PrivateKey(Ctx.get(), nullptr)));
    uint8_t *Temp = Res.data();
    opensslAssuming(i2d_PrivateKey(Ctx.get(), &Temp));
    return Res;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
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
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    EVP_PKEY *Kp = nullptr;
    ensureOrReturn(Encoded.size() == EddsaKeyPair::Size,
                   __WASI_CRYPTO_ERRNO_INVALID_KEY);

    const uint8_t *Temp = Encoded.data();
    ensureOrReturn(Encoded.size() <= LONG_MAX,
                   __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
    Kp = d2i_PrivateKey(EVP_PKEY_ED25519, &Kp, &Temp,
                        static_cast<long>(Encoded.size()));
    ensureOrReturn(Kp, __WASI_CRYPTO_ERRNO_INVALID_KEY);
    return std::make_unique<EddsaKeyPair>(Kp);
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<std::vector<uint8_t>>
EddsaKeyPair::exportData(__wasi_keypair_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    std::vector<uint8_t> Res(
        static_cast<size_t>(i2d_PrivateKey(Ctx.get(), nullptr)));
    uint8_t *Temp = Res.data();
    opensslAssuming(i2d_PrivateKey(Ctx.get(), &Temp));
    return Res;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
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
    ensureOrReturn(Encoded.size() == EddsaSignature::Size,
                   __WASI_CRYPTO_ERRNO_INVALID_SIGNATURE);
    return std::make_unique<EddsaSignature>(
        std::vector<uint8_t>{Encoded.begin(), Encoded.end()});
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<std::vector<uint8_t>>
EddsaSignature::exportData(__wasi_signature_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW:
    return Data;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<void> EddsaSignState::update(Span<const uint8_t> Input) {
  // Notice: Ecdsa is oneshot in OpenSSL, we need a cache for update instead of
  // call `EVP_DigestSignUpdate`
  std::unique_lock<std::shared_mutex> Lock{Mutex};
  Cache.insert(Cache.end(), Input.begin(), Input.end());
  return {};
}

WasiCryptoExpect<std::unique_ptr<Signature>> EddsaSignState::sign() {
  std::shared_lock<std::shared_mutex> Lock{Mutex};
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
  std::unique_lock<std::shared_mutex> Lock{Mutex};
  Cache.insert(Cache.end(), Input.begin(), Input.end());
  return {};
}

WasiCryptoExpect<void>
EddsaVerificationState::verify(std::shared_ptr<Signature> Sig) {
  std::shared_lock<std::shared_mutex> Lock{Mutex};

  ensureOrReturn(Sig->alg() == SignatureAlgorithm::Ed25519,
                 __WASI_CRYPTO_ERRNO_INVALID_SIGNATURE);

  ensureOrReturn(Sig->data().size() == EddsaSignature::Size,
                 __WASI_CRYPTO_ERRNO_INVALID_SIGNATURE);

  // The call to EVP_DigestVerifyFinal() internally finalizes a copy of the
  // digest context. This means that EVP_VerifyUpdate() and EVP_VerifyFinal()
  // can be called later to digest and verify additional data.
  ensureOrReturn(EVP_DigestVerify(Ctx.get(), Sig->data().data(),
                                  Sig->data().size(), Cache.data(),
                                  Cache.size()),
                 __WASI_CRYPTO_ERRNO_VERIFICATION_FAILED);

  return {};
}

} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
