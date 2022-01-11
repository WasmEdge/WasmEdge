// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/rsa.h"
#include <openssl/x509.h>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

namespace {

EVP_PKEY *initRsa(int Pad, int Size, int Sha) {
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> PCtx{
      EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr)};
  opensslAssuming(EVP_PKEY_CTX_set_rsa_padding(PCtx.get(), Pad));
  opensslAssuming(EVP_PKEY_CTX_set_rsa_keygen_bits(PCtx.get(), Size));
  opensslAssuming(EVP_PKEY_CTX_set_signature_md(PCtx.get(), ShaMap.at(Sha)));
  opensslAssuming(PCtx);
  EVP_PKEY *ED = nullptr;
  opensslAssuming(EVP_PKEY_paramgen(PCtx.get(), &ED));
  return ED;
}

} // namespace

// raw secret scalar encoded as big endian, SEC-1, compressed SEC-1, unencrypted
// PKCS#8, PEM-encoded unencrypted PKCS#8
template <int Pad, int Size, int Sha>
WasiCryptoExpect<std::unique_ptr<typename Rsa<Pad, Size, Sha>::PublicKey>>
Rsa<Pad, Size, Sha>::PublicKey::import(Span<const uint8_t> Encoded,
                                       __wasi_publickey_encoding_e_t Encoding) {
  EVP_PKEY *P = initRsa(Pad, Size, Sha);

  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    const uint8_t *Temp = Encoded.data();
    P = d2i_PublicKey(EVP_PKEY_RSA, &P, &Temp, Encoded.size());
    opensslAssuming(P);
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

  return std::make_unique<PublicKey>(P);
}

template <int Pad, int Size, int Sha>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<Pad, Size, Sha>::PublicKey::exportData(
    __wasi_publickey_encoding_e_t Encoding) {
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
    assumingUnreachable();
  }
}

template <int Pad, int Size, int Sha>
WasiCryptoExpect<std::unique_ptr<VerificationState>>
Rsa<Pad, Size, Sha>::PublicKey::openVerificationState() {
  EVP_MD_CTX *SignCtx = EVP_MD_CTX_create();
  opensslAssuming(SignCtx);
  opensslAssuming(
      EVP_DigestVerifyInit(SignCtx, nullptr, EVP_sha256(), nullptr, Pk.get()));

  return std::make_unique<VerificationState>(SignCtx);
}

template <int Pad, int Size, int Sha>
WasiCryptoExpect<std::unique_ptr<typename Rsa<Pad, Size, Sha>::SecretKey>>
Rsa<Pad, Size, Sha>::SecretKey::import(Span<const uint8_t> Encoded,
                                       __wasi_secretkey_encoding_e_t Encoding) {
  EVP_PKEY *Sk = initRsa(Pad, Size, Sha);

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
    assumingUnreachable();
  }

  return std::make_unique<SecretKey>(Sk);
}

template <int Pad, int Size, int Sha>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<Pad, Size, Sha>::SecretKey::exportData(
    __wasi_secretkey_encoding_e_t Encoding) {
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

template <int Pad, int Size, int Sha>
WasiCryptoExpect<std::unique_ptr<typename Rsa<Pad, Size, Sha>::KeyPair>>
Rsa<Pad, Size, Sha>::KeyPair::import(Span<const uint8_t> Encoded,
                                     __wasi_keypair_encoding_e_t Encoding) {
  EVP_PKEY *Kp = initRsa(Pad, Size, Sha);

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
    assumingUnreachable();
  }

  return std::make_unique<KeyPair>(Kp);
}

template <int Pad, int Size, int Sha>
WasiCryptoExpect<std::unique_ptr<Signatures::SignState>>
Rsa<Pad, Size, Sha>::KeyPair::openSignState() {
  EVP_MD_CTX *SignCtx = EVP_MD_CTX_create();
  opensslAssuming(SignCtx);

  opensslAssuming(
      EVP_DigestSignInit(SignCtx, nullptr, EVP_sha256(), nullptr, Kp.get()));
  return std::make_unique<SignState>(SignCtx);
}

template <int Pad, int Size, int Sha>
WasiCryptoExpect<std::unique_ptr<typename Rsa<Pad, Size, Sha>::KeyPair>>
Rsa<Pad, Size, Sha>::KeyPair::generate(std::shared_ptr<Options>) {
  EVP_PKEY_CTX *Ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
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

  return std::make_unique<KeyPair>(PKey);
}

template <int Pad, int Size, int Sha>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<Pad, Size, Sha>::KeyPair::exportData(__wasi_keypair_encoding_e_t Encoding) {
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
    assumingUnreachable();
  }
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int Pad, int Size, int Sha>
WasiCryptoExpect<std::unique_ptr<Signatures::PublicKey>>
Rsa<Pad, Size, Sha>::KeyPair::publicKey() {
  OpenSSLUniquePtr<BIO, BIO_free> B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PUBKEY_bio(B.get(), Kp.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PUBKEY_bio(B.get(), &Res));

  return std::make_unique<PublicKey>(Res);
}

template <int Pad, int Size, int Sha>
WasiCryptoExpect<std::unique_ptr<Signatures::SecretKey>>
Rsa<Pad, Size, Sha>::KeyPair::secretKey() {
  OpenSSLUniquePtr<BIO, BIO_free> B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PrivateKey_bio(B.get(), Kp.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PrivateKey_bio(B.get(), &Res));

  return std::make_unique<SecretKey>(Res);
}

template <int Pad, int Size, int Sha>
WasiCryptoExpect<std::unique_ptr<typename Rsa<Pad, Size, Sha>::Signature>>
Rsa<Pad, Size, Sha>::Signature::import(Span<const uint8_t> Encoded,
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

template <int Pad, int Size, int Sha>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<Pad, Size, Sha>::Signature::exportData(
    __wasi_signature_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW:
    return Sign;
  case __WASI_SIGNATURE_ENCODING_DER:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  default:
    assumingUnreachable();
  }
}

template <int Pad, int Size, int Sha>
WasiCryptoExpect<void>
Rsa<Pad, Size, Sha>::SignState::update(Span<const uint8_t> Data) {
  opensslAssuming(EVP_DigestSignUpdate(MdCtx.get(), Data.data(), Data.size()));
  return {};
}

template <int Pad, int Size, int Sha>
WasiCryptoExpect<std::unique_ptr<Signatures::Signature>>
Rsa<Pad, Size, Sha>::SignState::sign() {
  size_t Siz;
  opensslAssuming(EVP_DigestSignFinal(MdCtx.get(), nullptr, &Siz));

  std::vector<uint8_t> Res(Siz);

  opensslAssuming(EVP_DigestSignFinal(MdCtx.get(), Res.data(), &Siz));

  return std::make_unique<Signature>(std::move(Res));
}

template <int Pad, int Size, int Sha>
WasiCryptoExpect<void>
Rsa<Pad, Size, Sha>::VerificationState::update(Span<const uint8_t> Data) {
  opensslAssuming(
      EVP_DigestVerifyUpdate(MdCtx.get(), Data.data(), Data.size()));
  return {};
}

template <int Pad, int Size, int Sha>
WasiCryptoExpect<void> Rsa<Pad, Size, Sha>::VerificationState::verify(
    std::shared_ptr<Signatures::Signature> Sig) {
  auto Data = Sig->exportData(__WASI_SIGNATURE_ENCODING_RAW);
  if (!Data) {
    return WasiCryptoUnexpect(Data);
  }

  opensslAssuming(
      EVP_DigestVerifyFinal(MdCtx.get(), Data->data(), Data->size()));

  return {};
}

template class Rsa<RSA_PKCS1_PADDING, 2048, 256>;
template class Rsa<RSA_PKCS1_PADDING, 2048, 384>;
template class Rsa<RSA_PKCS1_PADDING, 2048, 512>;

template class Rsa<RSA_PKCS1_PADDING, 3072, 384>;
template class Rsa<RSA_PKCS1_PADDING, 3072, 512>;

template class Rsa<RSA_PKCS1_PADDING, 4096, 512>;

template class Rsa<RSA_PKCS1_PSS_PADDING, 2048, 256>;
template class Rsa<RSA_PKCS1_PSS_PADDING, 2048, 384>;
template class Rsa<RSA_PKCS1_PSS_PADDING, 2048, 512>;

template class Rsa<RSA_PKCS1_PSS_PADDING, 3072, 384>;
template class Rsa<RSA_PKCS1_PSS_PADDING, 3072, 512>;

template class Rsa<RSA_PKCS1_PSS_PADDING, 4096, 512>;

} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
