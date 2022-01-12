// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/ecdsa.h"

#include "openssl/ec.h"
#include "openssl/x509.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {
namespace {

// TODO:Raw meaning compressed_sec not sec, please check

EVP_PKEY *initEC(int Nid) {
  EvpPkeyCtxPtr PCtx{EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr)};
  opensslAssuming(PCtx);
  opensslAssuming(EVP_PKEY_paramgen_init(PCtx.get()));
  opensslAssuming(EVP_PKEY_CTX_set_ec_paramgen_curve_nid(PCtx.get(), Nid));

  EVP_PKEY *Params = nullptr;
  opensslAssuming(EVP_PKEY_paramgen(PCtx.get(), &Params));
  return Params;
}

} // namespace

// raw secret scalar encoded as big endian, SEC-1, compressed SEC-1, unencrypted
// PKCS#8, PEM-encoded unencrypted PKCS#8
template <int Nid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<Nid>::PublicKey>>
Ecdsa<Nid>::PublicKey::import(Span<const uint8_t> Encoded,
                              __wasi_publickey_encoding_e_t Encoding) {
  EVP_PKEY *Pk = initEC(Nid);
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
    assumingUnreachable();
  }

  return std::make_unique<PublicKey>(Pk);
}

template <int Nid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<Nid>::PublicKey::exportData(__wasi_publickey_encoding_e_t Encoding) {
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

template <int Nid>
WasiCryptoExpect<std::unique_ptr<Signatures::VerificationState>>
Ecdsa<Nid>::PublicKey::openVerificationState() {
  EVP_MD_CTX *SignCtx = EVP_MD_CTX_create();
  opensslAssuming(SignCtx);
  opensslAssuming(
      EVP_DigestVerifyInit(SignCtx, nullptr, EVP_sha256(), nullptr, Ctx.get()));

  return std::make_unique<VerificationState>(SignCtx);
}

template <int Nid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<Nid>::SecretKey>>
Ecdsa<Nid>::SecretKey::import(Span<const uint8_t> Encoded,
                              __wasi_secretkey_encoding_e_t Encoding) {
  EVP_PKEY *Sk = initEC(Nid);
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
    assumingUnreachable();
  }

  return std::make_unique<SecretKey>(Sk);
}

template <int Nid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<Nid>::SecretKey::exportData(__wasi_secretkey_encoding_e_t Encoding) {
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

template <int Nid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<Nid>::KeyPair>>
Ecdsa<Nid>::KeyPair::generate(std::shared_ptr<Options>) {
  EVP_PKEY *Params = initEC(Nid);

  // Generate Key
  EvpPkeyCtxPtr KCtx{EVP_PKEY_CTX_new(Params, nullptr)};
  opensslAssuming(KCtx);
  opensslAssuming(EVP_PKEY_keygen_init(KCtx.get()));

  EVP_PKEY *Key = nullptr;
  opensslAssuming(EVP_PKEY_keygen(KCtx.get(), &Key));

  return std::make_unique<KeyPair>(Key);
}

template <int Nid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<Nid>::KeyPair>>
Ecdsa<Nid>::KeyPair::import(Span<const uint8_t> Encoded,
                            __wasi_keypair_encoding_e_t Encoding) {
  EVP_PKEY *Kp = initEC(Nid);
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
    assumingUnreachable();
  }

  return std::make_unique<KeyPair>(Kp);
}

template <int Nid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<Nid>::KeyPair::exportData(__wasi_keypair_encoding_e_t Encoding) {
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

template <int Nid>
WasiCryptoExpect<std::unique_ptr<Signatures::PublicKey>>
Ecdsa<Nid>::KeyPair::publicKey() {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PUBKEY_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PUBKEY_bio(B.get(), &Res));

  return std::make_unique<PublicKey>(Res);
}

template <int Nid>
WasiCryptoExpect<std::unique_ptr<SecretKey>> Ecdsa<Nid>::KeyPair::secretKey() {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PrivateKey_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PrivateKey_bio(B.get(), &Res));

  return std::make_unique<SecretKey>(Res);
}

template <int Nid>
WasiCryptoExpect<std::unique_ptr<SignState>>
Ecdsa<Nid>::KeyPair::openSignState() {
  EVP_MD_CTX *SignCtx = EVP_MD_CTX_create();
  opensslAssuming(SignCtx);

  opensslAssuming(
      EVP_DigestSignInit(SignCtx, nullptr, EVP_sha256(), nullptr, Ctx.get()));

  return std::make_unique<SignState>(SignCtx);
}

template <int Nid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<Nid>::Signature>>
Ecdsa<Nid>::Signature::import(Span<const uint8_t> Encoded,
                              __wasi_signature_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW:
    return std::make_unique<Signature>(
        std::vector<uint8_t>{Encoded.begin(), Encoded.end()});
  case __WASI_SIGNATURE_ENCODING_DER:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  default:
    assumingUnreachable();
  }
}

template <int Nid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<Nid>::Signature::exportData(__wasi_signature_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW:
    return Data;
  case __WASI_SIGNATURE_ENCODING_DER:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  default:
    assumingUnreachable();
  }
}

template <int Nid>
WasiCryptoExpect<void> Ecdsa<Nid>::SignState::update(Span<const uint8_t> Data) {
  opensslAssuming(EVP_DigestSignUpdate(Ctx.get(), Data.data(), Data.size()));

  return {};
}

template <int Nid>
WasiCryptoExpect<std::unique_ptr<Signature>> Ecdsa<Nid>::SignState::sign() {
  size_t Size;
  opensslAssuming(EVP_DigestSignFinal(Ctx.get(), nullptr, &Size));

  std::vector<uint8_t> Res(Size);
  opensslAssuming(EVP_DigestSignFinal(Ctx.get(), Res.data(), &Size));

  return std::make_unique<Signature>(std::move(Res));
}

template <int Nid>
WasiCryptoExpect<void>
Ecdsa<Nid>::VerificationState::update(Span<const uint8_t> Data) {
  opensslAssuming(EVP_DigestVerifyUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

template <int Nid>
WasiCryptoExpect<void> Ecdsa<Nid>::VerificationState::verify(
    std::shared_ptr<Signatures::Signature> Sig) {
  ensureOrReturn(Sig->alg() == getAlg(), __WASI_CRYPTO_ERRNO_INVALID_SIGNATURE);

  opensslAssuming(
      EVP_DigestVerifyFinal(Ctx.get(), Sig->data().data(), Sig->data().size()));
  return {};
}

template class Ecdsa<NID_X9_62_prime256v1>;
template class Ecdsa<NID_secp256k1>;
} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
