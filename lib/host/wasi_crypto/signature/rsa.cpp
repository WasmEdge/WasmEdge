// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/rsa.h"

#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/evpwrapper.h"
#include "host/wasi_crypto/signature/secretkey.h"
#include "openssl/x509.h"
#include "wasi_crypto/api.hpp"
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

// raw secret scalar encoded as big endian, SEC-1, compressed SEC-1, unencrypted
// PKCS#8, PEM-encoded unencrypted PKCS#8
template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<typename Rsa<Pad, Size, Sha>::PublicKey>>
Rsa<Pad, Size, Sha>::PublicKey::import(Span<const uint8_t> Encoded,
                                       __wasi_publickey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_PKCS8:
    return importPkcs8(Encoded);
  case __WASI_PUBLICKEY_ENCODING_PEM:
    return importPem(Encoded);
  case __WASI_PUBLICKEY_ENCODING_LOCAL:
    return importLocal(Encoded);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<typename Rsa<Pad, Size, Sha>::PublicKey>>
Rsa<Pad, Size, Sha>::PublicKey::importPkcs8(Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};
  BIO_write(Bio.get(), Encoded.data(), Encoded.size());

  auto InitCtx = initRsa();
  if (!InitCtx) {
    return WasiCryptoUnexpect(InitCtx);
  }

  EVP_PKEY *P = *InitCtx;
  P = d2i_PKCS8PrivateKey_bio(Bio.get(), &P, nullptr, nullptr);
  ensureOrReturn(P, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return std::make_unique<PublicKey>(P);
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<typename Rsa<Pad, Size, Sha>::PublicKey>>
Rsa<Pad, Size, Sha>::PublicKey::importPem(Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};
  BIO_write(Bio.get(), Encoded.data(), Encoded.size());

  auto InitCtx = initRsa();
  if (!InitCtx) {
    return WasiCryptoUnexpect(InitCtx);
  }

  EVP_PKEY *P = *InitCtx;
  P = PEM_read_bio_PUBKEY(Bio.get(), &P, nullptr, nullptr);
  ensureOrReturn(P, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return std::make_unique<PublicKey>(P);
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<typename Rsa<Pad, Size, Sha>::PublicKey>>
Rsa<Pad, Size, Sha>::PublicKey::importLocal(Span<const uint8_t> Encoded) {
  auto InitCtx = initRsa();
  if (!InitCtx) {
    return WasiCryptoUnexpect(InitCtx);
  }

  EVP_PKEY *P = *InitCtx;
  const uint8_t *Temp = Encoded.data();
  ensureOrReturn(Encoded.size() <= LONG_MAX,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  P = d2i_PublicKey(EVP_PKEY_RSA, &P, &Temp, static_cast<long>(Encoded.size()));
  ensureOrReturn(P, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return std::make_unique<PublicKey>(P);
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<Pad, Size, Sha>::PublicKey::exportData(
    __wasi_publickey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_PKCS8:
    return exportPkcs8();
  case __WASI_PUBLICKEY_ENCODING_PEM:
    return exportPem();
  case __WASI_PUBLICKEY_ENCODING_LOCAL:
    return exportLocal();
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<Pad, Size, Sha>::PublicKey::exportPkcs8() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<Pad, Size, Sha>::PublicKey::exportPem() {
  BioPtr Bio{BIO_new(BIO_s_mem())};
  PEM_write_bio_PUBKEY(Bio.get(), Pk.get());

  char *Temp = nullptr;
  int Siz = BIO_get_mem_data(Bio.get(), Temp);
  std::vector<uint8_t> Res(static_cast<size_t>(Siz));
  ensureOrReturn(BIO_read(Bio.get(), Res.data(), Siz),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return Res;
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<Pad, Size, Sha>::PublicKey::exportLocal() {
  std::vector<uint8_t> Res(
      static_cast<size_t>(i2d_PublicKey(Pk.get(), nullptr)));
  uint8_t *Temp = Res.data();
  opensslAssuming(i2d_PublicKey(Pk.get(), &Temp));
  return Res;
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<VerificationState>>
Rsa<Pad, Size, Sha>::PublicKey::openVerificationState() {
  EVP_MD_CTX *SignCtx = EVP_MD_CTX_create();
  opensslAssuming(SignCtx);
  opensslAssuming(
      EVP_DigestVerifyInit(SignCtx, nullptr, EVP_sha256(), nullptr, Pk.get()));

  return std::make_unique<VerificationState>(SignCtx);
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<typename Rsa<Pad, Size, Sha>::SecretKey>>
Rsa<Pad, Size, Sha>::SecretKey::import(Span<const uint8_t> Encoded,
                                       __wasi_secretkey_encoding_e_t Encoding) {

  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_PKCS8:
    return importPkcs8(Encoded);
  case __WASI_SECRETKEY_ENCODING_PEM:
    return importPem(Encoded);
  case __WASI_SECRETKEY_ENCODING_LOCAL:
    return importLocal(Encoded);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<typename Rsa<Pad, Size, Sha>::SecretKey>>
Rsa<Pad, Size, Sha>::SecretKey::importPem(Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};
  BIO_write(Bio.get(), Encoded.data(), Encoded.size());

  auto InitCtx = initRsa();
  if (!InitCtx) {
    return WasiCryptoUnexpect(InitCtx);
  }

  EVP_PKEY *P = *InitCtx;
  P = PEM_read_bio_PrivateKey(Bio.get(), &P, nullptr, nullptr);
  ensureOrReturn(P, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return std::make_unique<SecretKey>(P);
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<typename Rsa<Pad, Size, Sha>::SecretKey>>
Rsa<Pad, Size, Sha>::SecretKey::importPkcs8(Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<typename Rsa<Pad, Size, Sha>::SecretKey>>
Rsa<Pad, Size, Sha>::SecretKey::importLocal(Span<const uint8_t> Encoded) {
  auto InitCtx = initRsa();
  if (!InitCtx) {
    return WasiCryptoUnexpect(InitCtx);
  }

  EVP_PKEY *Sk = *InitCtx;

  const uint8_t *Temp = Encoded.data();
  ensureOrReturn(Encoded.size() <= LONG_MAX,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  Sk = d2i_PrivateKey(EVP_PKEY_RSA, &Sk, &Temp,
                      static_cast<long>(Encoded.size()));
  ensureOrReturn(Sk, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return std::make_unique<SecretKey>(Sk);
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<Pad, Size, Sha>::SecretKey::exportData(
    __wasi_secretkey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_PKCS8:
    return exportPkcs8();
  case __WASI_SECRETKEY_ENCODING_PEM:
    return exportPem();
  case __WASI_SECRETKEY_ENCODING_LOCAL:
    return exportLocal();
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<Pad, Size, Sha>::SecretKey::exportPem() {
  BioPtr Bio{BIO_new(BIO_s_mem())};
  ensureOrReturn(PEM_write_bio_PrivateKey(Bio.get(), Ctx.get(), nullptr,
                                          nullptr, 0, nullptr, nullptr),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  char *Temp = nullptr;
  int Siz = BIO_get_mem_data(Bio.get(), Temp);
  std::vector<uint8_t> Res(static_cast<size_t>(Siz));
  ensureOrReturn(BIO_read(Bio.get(), Res.data(), Siz),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return Res;
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<Pad, Size, Sha>::SecretKey::exportPkcs8() {
  BioPtr Bio{BIO_new(BIO_s_mem())};
  ensureOrReturn(PEM_write_bio_PKCS8PrivateKey(Bio.get(), Ctx.get(), nullptr,
                                               nullptr, 0, nullptr, nullptr),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  char *Temp = nullptr;
  int Siz = BIO_get_mem_data(Bio.get(), Temp);
  std::vector<uint8_t> Res(static_cast<size_t>(Siz));
  ensureOrReturn(BIO_read(Bio.get(), Res.data(), Siz),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return Res;
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<Pad, Size, Sha>::SecretKey::exportLocal() {
  std::vector<uint8_t> Res(
      static_cast<size_t>(i2d_PrivateKey(Ctx.get(), nullptr)));
  uint8_t *Temp = Res.data();
  opensslAssuming(i2d_PrivateKey(Ctx.get(), &Temp));
  return Res;
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<typename Rsa<Pad, Size, Sha>::KeyPair>>
Rsa<Pad, Size, Sha>::KeyPair::import(Span<const uint8_t> Encoded,
                                       __wasi_keypair_encoding_e_t Encoding) {

  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_PKCS8:
    return importPkcs8(Encoded);
  case __WASI_KEYPAIR_ENCODING_PEM:
    return importPem(Encoded);
  case __WASI_KEYPAIR_ENCODING_LOCAL:
    return importLocal(Encoded);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<typename Rsa<Pad, Size, Sha>::KeyPair>>
Rsa<Pad, Size, Sha>::KeyPair::importPem(Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};
  BIO_write(Bio.get(), Encoded.data(), Encoded.size());

  auto InitCtx = initRsa();
  if (!InitCtx) {
    return WasiCryptoUnexpect(InitCtx);
  }

  EVP_PKEY *P = *InitCtx;
  P = PEM_read_bio_PrivateKey(Bio.get(), &P, nullptr, nullptr);
  ensureOrReturn(P, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return std::make_unique<KeyPair>(P);
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<typename Rsa<Pad, Size, Sha>::KeyPair>>
Rsa<Pad, Size, Sha>::KeyPair::importPkcs8(Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<typename Rsa<Pad, Size, Sha>::KeyPair>>
Rsa<Pad, Size, Sha>::KeyPair::importLocal(Span<const uint8_t> Encoded) {
  auto InitCtx = initRsa();
  if (!InitCtx) {
    return WasiCryptoUnexpect(InitCtx);
  }

  EVP_PKEY *Sk = *InitCtx;

  const uint8_t *Temp = Encoded.data();
  ensureOrReturn(Encoded.size() <= LONG_MAX,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  Sk = d2i_PrivateKey(EVP_PKEY_RSA, &Sk, &Temp,
                      static_cast<long>(Encoded.size()));
  ensureOrReturn(Sk, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return std::make_unique<KeyPair>(Sk);
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<Signatures::SignState>>
Rsa<Pad, Size, Sha>::KeyPair::openSignState() {
  EVP_MD_CTX *SignCtx = EVP_MD_CTX_create();
  opensslAssuming(SignCtx);

  opensslAssuming(
      EVP_DigestSignInit(SignCtx, nullptr, EVP_sha256(), nullptr, Ctx.get()));
  return std::make_unique<SignState>(SignCtx);
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<typename Rsa<Pad, Size, Sha>::KeyPair>>
Rsa<Pad, Size, Sha>::KeyPair::generate(std::shared_ptr<Options>) {
  EVP_PKEY_CTX *Ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
  opensslAssuming(Ctx);

  opensslAssuming(EVP_PKEY_keygen_init(Ctx));
  EVP_PKEY *PKey = nullptr;
  opensslAssuming(EVP_PKEY_keygen(Ctx, &PKey));

  return std::make_unique<KeyPair>(PKey);
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<Pad, Size, Sha>::KeyPair::exportData(
    __wasi_keypair_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_PKCS8:
    return exportPkcs8();
  case __WASI_KEYPAIR_ENCODING_PEM:
    return exportPem();
  case __WASI_KEYPAIR_ENCODING_LOCAL:
    return exportLocal();
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<Pad, Size, Sha>::KeyPair::exportPem() {
  BioPtr Bio{BIO_new(BIO_s_mem())};
  ensureOrReturn(PEM_write_bio_PrivateKey(Bio.get(), Ctx.get(), nullptr,
                                          nullptr, 0, nullptr, nullptr),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  char *Temp = nullptr;
  int Siz = BIO_get_mem_data(Bio.get(), Temp);
  std::vector<uint8_t> Res(static_cast<size_t>(Siz));
  ensureOrReturn(BIO_read(Bio.get(), Res.data(), Siz),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return Res;
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<Pad, Size, Sha>::KeyPair::exportPkcs8() {
  BioPtr Bio{BIO_new(BIO_s_mem())};
  ensureOrReturn(PEM_write_bio_PKCS8PrivateKey(Bio.get(), Ctx.get(), nullptr,
                                               nullptr, 0, nullptr, nullptr),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  char *Temp = nullptr;
  int Siz = BIO_get_mem_data(Bio.get(), Temp);
  std::vector<uint8_t> Res(static_cast<size_t>(Siz));
  ensureOrReturn(BIO_read(Bio.get(), Res.data(), Siz),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return Res;
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<Pad, Size, Sha>::KeyPair::exportLocal() {
  std::vector<uint8_t> Res(
      static_cast<size_t>(i2d_PrivateKey(Ctx.get(), nullptr)));
  uint8_t *Temp = Res.data();
  opensslAssuming(i2d_PrivateKey(Ctx.get(), &Temp));
  return Res;
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<Signatures::PublicKey>>
Rsa<Pad, Size, Sha>::KeyPair::publicKey() {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PUBKEY_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PUBKEY_bio(B.get(), &Res));

  return std::make_unique<PublicKey>(Res);
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<Signatures::SecretKey>>
Rsa<Pad, Size, Sha>::KeyPair::secretKey() {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PrivateKey_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PrivateKey_bio(B.get(), &Res));

  return std::make_unique<SecretKey>(Res);
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<typename Rsa<Pad, Size, Sha>::Signature>>
Rsa<Pad, Size, Sha>::Signature::import(Span<const uint8_t> Encoded,
                                       __wasi_signature_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW:
    ensureOrReturn(Encoded.size() == Size / 8,
                   __WASI_CRYPTO_ERRNO_INVALID_SIGNATURE);
    return std::make_unique<Signature>(
        std::vector<uint8_t>{Encoded.begin(), Encoded.end()});
  case __WASI_SIGNATURE_ENCODING_DER:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  default:
    assumingUnreachable();
  }
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<Pad, Size, Sha>::Signature::exportData(
    __wasi_signature_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW:
    return Data;
  case __WASI_SIGNATURE_ENCODING_DER:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  default:
    assumingUnreachable();
  }
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<void>
Rsa<Pad, Size, Sha>::SignState::update(Span<const uint8_t> Data) {
  opensslAssuming(EVP_DigestSignUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<std::unique_ptr<Signatures::Signature>>
Rsa<Pad, Size, Sha>::SignState::sign() {
  size_t Siz;
  opensslAssuming(EVP_DigestSignFinal(Ctx.get(), nullptr, &Siz));

  std::vector<uint8_t> Res(Siz);

  opensslAssuming(EVP_DigestSignFinal(Ctx.get(), Res.data(), &Siz));

  return std::make_unique<Signature>(std::move(Res));
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<void>
Rsa<Pad, Size, Sha>::VerificationState::update(Span<const uint8_t> Data) {
  opensslAssuming(EVP_DigestVerifyUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<void> Rsa<Pad, Size, Sha>::VerificationState::verify(
    std::shared_ptr<Signatures::Signature> Sig) {
  ensureOrReturn(Sig->alg() == getAlg(), __WASI_CRYPTO_ERRNO_INVALID_SIGNATURE);

  ensureOrReturn(
      EVP_DigestVerifyFinal(Ctx.get(), Sig->data().data(), Sig->data().size()),
      __WASI_CRYPTO_ERRNO_VERIFICATION_FAILED);

  return {};
}

template <uint32_t Pad, uint32_t Size, uint32_t Sha>
WasiCryptoExpect<EVP_PKEY *> Rsa<Pad, Size, Sha>::initRsa() {
  EvpPkeyCtxPtr PCtx{EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr)};
  opensslAssuming(EVP_PKEY_CTX_set_rsa_padding(PCtx.get(), Pad));
  opensslAssuming(EVP_PKEY_CTX_set_rsa_keygen_bits(PCtx.get(), Size));
  // ugly pass error
  opensslAssuming(EVP_PKEY_CTX_set_signature_md(
      PCtx.get(), static_cast<void *>(const_cast<EVP_MD *>(ShaMap.at(Sha)))));
  opensslAssuming(PCtx);
  EVP_PKEY *ED = nullptr;
  opensslAssuming(EVP_PKEY_paramgen(PCtx.get(), &ED));
  return ED;
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
