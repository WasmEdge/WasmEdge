// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/ecdsa.h"

#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/evpwrapper.h"
#include "host/wasi_crypto/signature/publickey.h"
#include "openssl/bio.h"
#include "openssl/ec.h"
#include "openssl/pem.h"
#include "openssl/x509.h"
#include "wasi_crypto/api.hpp"
#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

// raw secret scalar encoded as big endian, SEC-1, compressed SEC-1, unencrypted
// PKCS#8, PEM-encoded unencrypted PKCS#8
template <uint32_t Nid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<Nid>::PublicKey>>
Ecdsa<Nid>::PublicKey::import(Span<const uint8_t> Encoded,
                              __wasi_publickey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW:
    return importCompressedSec(Encoded);
  case __WASI_PUBLICKEY_ENCODING_PKCS8:
    return importPkcs8(Encoded);
  case __WASI_PUBLICKEY_ENCODING_PEM:
    return importPem(Encoded);
  case __WASI_PUBLICKEY_ENCODING_SEC:
    return importSec(Encoded);
  case __WASI_PUBLICKEY_ENCODING_COMPRESSED_SEC:
    return importCompressedSec(Encoded);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <uint32_t Nid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<Nid>::PublicKey::exportData(__wasi_publickey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW:
    return exportCompressedSec();
  case __WASI_PUBLICKEY_ENCODING_PKCS8:
    return exportPkcs8();
  case __WASI_PUBLICKEY_ENCODING_PEM:
    return exportPem();
  case __WASI_PUBLICKEY_ENCODING_SEC:
    return exportSec();
  case __WASI_PUBLICKEY_ENCODING_COMPRESSED_SEC:
    return exportCompressedSec();
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <uint32_t Nid>
WasiCryptoExpect<std::unique_ptr<Signatures::VerificationState>>
Ecdsa<Nid>::PublicKey::openVerificationState() {
  EVP_MD_CTX *SignCtx = EVP_MD_CTX_create();
  opensslAssuming(SignCtx);
  opensslAssuming(
      EVP_DigestVerifyInit(SignCtx, nullptr, EVP_sha256(), nullptr, Ctx.get()));

  return std::make_unique<VerificationState>(SignCtx);
}

template <uint32_t Nid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<Nid>::PublicKey>>
Ecdsa<Nid>::PublicKey::importPkcs8(Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <uint32_t Nid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<Nid>::PublicKey>>
Ecdsa<Nid>::PublicKey::importPem(Span<const uint8_t> Encoded) {
  auto InitCtx = initEC();
  if (!InitCtx) {
    return WasiCryptoUnexpect(InitCtx);
  }

  EVP_PKEY *Pk = *InitCtx;

  BioPtr Bio{BIO_new(BIO_s_mem())};
  ensureOrReturn(BIO_write(Bio.get(), Encoded.data(), Encoded.size()),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  ensureOrReturn(PEM_read_bio_PUBKEY(Bio.get(), &Pk, nullptr, nullptr),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return std::make_unique<PublicKey>(Pk);
}

template <uint32_t Nid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<Nid>::PublicKey>>
Ecdsa<Nid>::PublicKey::importSec(Span<const uint8_t> Encoded) {
  auto InitCtx = initEC();
  if (!InitCtx) {
    return WasiCryptoUnexpect(InitCtx);
  }

  EVP_PKEY *Pk = *InitCtx;
  EC_KEY_set_conv_form(EVP_PKEY_get0_EC_KEY(Pk), POINT_CONVERSION_UNCOMPRESSED);
  const uint8_t *Temp = Encoded.data();
  ensureOrReturn(Encoded.size() <= LONG_MAX,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  Pk =
      d2i_PublicKey(EVP_PKEY_EC, &Pk, &Temp, static_cast<long>(Encoded.size()));
  ensureOrReturn(Pk, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return std::make_unique<PublicKey>(Pk);
}

template <uint32_t Nid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<Nid>::PublicKey>>
Ecdsa<Nid>::PublicKey::importCompressedSec(Span<const uint8_t> Encoded) {
  auto InitCtx = initEC();
  if (!InitCtx) {
    return WasiCryptoUnexpect(InitCtx);
  }

  EVP_PKEY *Pk = *InitCtx;
  const uint8_t *Temp = Encoded.data();
  ensureOrReturn(Encoded.size() <= LONG_MAX,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  EC_KEY_set_conv_form(EVP_PKEY_get0_EC_KEY(Pk), POINT_CONVERSION_COMPRESSED);
  Pk =
      d2i_PublicKey(EVP_PKEY_EC, &Pk, &Temp, static_cast<long>(Encoded.size()));
  ensureOrReturn(Pk, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return std::make_unique<PublicKey>(Pk);
}

template <uint32_t Nid>
WasiCryptoExpect<std::vector<uint8_t>> Ecdsa<Nid>::PublicKey::exportPkcs8() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <uint32_t Nid>
WasiCryptoExpect<std::vector<uint8_t>> Ecdsa<Nid>::PublicKey::exportPem() {
  BioPtr Bio{BIO_new(BIO_s_mem())};
  ensureOrReturn(PEM_write_bio_PUBKEY(Bio.get(), Ctx.get()),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  char *Temp = nullptr;
  int Siz = BIO_get_mem_data(Bio.get(), Temp);
  std::vector<uint8_t> Res(static_cast<size_t>(Siz));
  ensureOrReturn(BIO_read(Bio.get(), Res.data(), Siz),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return Res;
}

template <uint32_t Nid>
WasiCryptoExpect<std::vector<uint8_t>> Ecdsa<Nid>::PublicKey::exportSec() {
  EC_KEY_set_conv_form(EVP_PKEY_get0_EC_KEY(Ctx.get()),
                       POINT_CONVERSION_UNCOMPRESSED);
  std::vector<uint8_t> Res(
      static_cast<size_t>(i2d_PublicKey(Ctx.get(), nullptr)));
  uint8_t *Temp = Res.data();
  opensslAssuming(i2d_PublicKey(Ctx.get(), &Temp));
  return Res;
}

template <uint32_t Nid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<Nid>::PublicKey::exportCompressedSec() {
  EC_KEY_set_conv_form(EVP_PKEY_get0_EC_KEY(Ctx.get()),
                       POINT_CONVERSION_COMPRESSED);
  std::vector<uint8_t> Res(
      static_cast<size_t>(i2d_PublicKey(Ctx.get(), nullptr)));
  uint8_t *Temp = Res.data();
  opensslAssuming(i2d_PublicKey(Ctx.get(), &Temp));
  return Res;
}

template <uint32_t Nid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<Nid>::SecretKey>>
Ecdsa<Nid>::SecretKey::import(Span<const uint8_t> Encoded,
                              __wasi_secretkey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW:
    return importCompressedSec(Encoded);
  case __WASI_SECRETKEY_ENCODING_PKCS8:
    return importPkcs8(Encoded);
  case __WASI_SECRETKEY_ENCODING_PEM:
    return importPem(Encoded);
  case __WASI_SECRETKEY_ENCODING_SEC:
    return importSec(Encoded);
  case __WASI_SECRETKEY_ENCODING_COMPRESSED_SEC:
    return importCompressedSec(Encoded);
  case __WASI_SECRETKEY_ENCODING_LOCAL:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  }
}

template <uint32_t Nid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<Nid>::SecretKey>>
Ecdsa<Nid>::SecretKey::importPkcs8(Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <uint32_t Nid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<Nid>::SecretKey>>
Ecdsa<Nid>::SecretKey::importPem(Span<const uint8_t> Encoded) {
  auto InitCtx = initEC();
  if (!InitCtx) {
    return WasiCryptoUnexpect(InitCtx);
  }

  EVP_PKEY *Sk = *InitCtx;

  BioPtr Bio{BIO_new(BIO_s_mem())};
  ensureOrReturn(BIO_write(Bio.get(), Encoded.data(), Encoded.size()),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  ensureOrReturn(PEM_read_bio_PrivateKey(Bio.get(), &Sk, nullptr, nullptr),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return std::make_unique<SecretKey>(Sk);
}

template <uint32_t Nid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<Nid>::SecretKey>>
Ecdsa<Nid>::SecretKey::importSec(Span<const uint8_t> Encoded) {
  auto InitCtx = initEC();
  if (!InitCtx) {
    return WasiCryptoUnexpect(InitCtx);
  }

  EVP_PKEY *Sk = *InitCtx;
  EC_KEY_set_conv_form(EVP_PKEY_get0_EC_KEY(Sk), POINT_CONVERSION_UNCOMPRESSED);
  const uint8_t *Temp = Encoded.data();
  ensureOrReturn(Encoded.size() <= LONG_MAX,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  Sk = d2i_PrivateKey(EVP_PKEY_EC, &Sk, &Temp,
                      static_cast<long>(Encoded.size()));
  ensureOrReturn(Sk, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return std::make_unique<SecretKey>(Sk);
}

template <uint32_t Nid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<Nid>::SecretKey>>
Ecdsa<Nid>::SecretKey::importCompressedSec(Span<const uint8_t> Encoded) {
  auto InitCtx = initEC();
  if (!InitCtx) {
    return WasiCryptoUnexpect(InitCtx);
  }

  EVP_PKEY *Sk = *InitCtx;
  EC_KEY_set_conv_form(EVP_PKEY_get0_EC_KEY(Sk), POINT_CONVERSION_COMPRESSED);
  const uint8_t *Temp = Encoded.data();
  ensureOrReturn(Encoded.size() <= LONG_MAX,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  Sk = d2i_PrivateKey(EVP_PKEY_EC, &Sk, &Temp,
                      static_cast<long>(Encoded.size()));
  ensureOrReturn(Sk, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return std::make_unique<SecretKey>(Sk);
}

template <uint32_t Nid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<Nid>::SecretKey::exportData(__wasi_secretkey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW:
    return exportCompressedSec();
  case __WASI_SECRETKEY_ENCODING_PKCS8:
    return exportPkcs8();
  case __WASI_SECRETKEY_ENCODING_PEM:
    return exportPem();
  case __WASI_SECRETKEY_ENCODING_SEC:
    return exportSec();
  case __WASI_SECRETKEY_ENCODING_COMPRESSED_SEC:
    return exportCompressedSec();
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}
template <uint32_t Nid>
WasiCryptoExpect<std::vector<uint8_t>> Ecdsa<Nid>::SecretKey::exportPkcs8() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <uint32_t Nid>
WasiCryptoExpect<std::vector<uint8_t>> Ecdsa<Nid>::SecretKey::exportPem() {
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

template <uint32_t Nid>
WasiCryptoExpect<std::vector<uint8_t>> Ecdsa<Nid>::SecretKey::exportSec() {
  EC_KEY_set_conv_form(EVP_PKEY_get0_EC_KEY(Ctx.get()),
                       POINT_CONVERSION_UNCOMPRESSED);
  std::vector<uint8_t> Res(
      static_cast<size_t>(i2d_PrivateKey(Ctx.get(), nullptr)));
  uint8_t *Temp = Res.data();
  opensslAssuming(i2d_PrivateKey(Ctx.get(), &Temp));
  return Res;
}

template <uint32_t Nid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<Nid>::SecretKey::exportCompressedSec() {
  EC_KEY_set_conv_form(EVP_PKEY_get0_EC_KEY(Ctx.get()),
                       POINT_CONVERSION_COMPRESSED);
  std::vector<uint8_t> Res(
      static_cast<size_t>(i2d_PrivateKey(Ctx.get(), nullptr)));
  uint8_t *Temp = Res.data();
  opensslAssuming(i2d_PrivateKey(Ctx.get(), &Temp));
  return Res;
}

template <uint32_t Nid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<Nid>::KeyPair>>
Ecdsa<Nid>::KeyPair::generate(std::shared_ptr<Options>) {
  auto InitCtx = initEC();
  if (!InitCtx) {
    return WasiCryptoUnexpect(InitCtx);
  }

  EVP_PKEY *Params = *InitCtx;

  // Generate Key
  EvpPkeyCtxPtr KCtx{EVP_PKEY_CTX_new(Params, nullptr)};
  opensslAssuming(KCtx);
  opensslAssuming(EVP_PKEY_keygen_init(KCtx.get()));

  EVP_PKEY *Key = nullptr;
  opensslAssuming(EVP_PKEY_keygen(KCtx.get(), &Key));

  return std::make_unique<KeyPair>(Key);
}

template <uint32_t Nid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<Nid>::KeyPair>>
Ecdsa<Nid>::KeyPair::import(Span<const uint8_t> Encoded,
                            __wasi_keypair_encoding_e_t Encoding) {

  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW:
    return importRaw(Encoded);
  case __WASI_KEYPAIR_ENCODING_PKCS8:
    return importPkcs8(Encoded);
  case __WASI_KEYPAIR_ENCODING_PEM:
    return importPem(Encoded);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <uint32_t Nid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<Nid>::KeyPair::exportData(__wasi_keypair_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW:
    return exportRaw();
  case __WASI_KEYPAIR_ENCODING_PKCS8:
    return exportPkcs8();
  case __WASI_KEYPAIR_ENCODING_PEM:
    return exportPem();
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <uint32_t Nid>
WasiCryptoExpect<std::unique_ptr<Signatures::PublicKey>>
Ecdsa<Nid>::KeyPair::publicKey() {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PUBKEY_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PUBKEY_bio(B.get(), &Res));

  return std::make_unique<PublicKey>(Res);
}

template <uint32_t Nid>
WasiCryptoExpect<std::unique_ptr<SecretKey>> Ecdsa<Nid>::KeyPair::secretKey() {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PrivateKey_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PrivateKey_bio(B.get(), &Res));

  return std::make_unique<SecretKey>(Res);
}

template <uint32_t Nid>
WasiCryptoExpect<std::unique_ptr<SignState>>
Ecdsa<Nid>::KeyPair::openSignState() {
  EVP_MD_CTX *SignCtx = EVP_MD_CTX_create();
  opensslAssuming(SignCtx);

  opensslAssuming(
      EVP_DigestSignInit(SignCtx, nullptr, EVP_sha256(), nullptr, Ctx.get()));

  return std::make_unique<SignState>(SignCtx);
}

template <uint32_t Nid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<Nid>::KeyPair>>
Ecdsa<Nid>::KeyPair::importPkcs8(Span<const uint8_t> Encoded) {
  auto InitCtx = initEC();
  if (!InitCtx) {
    return WasiCryptoUnexpect(InitCtx);
  }

  EVP_PKEY *Sk = *InitCtx;

  BioPtr Bio{BIO_new(BIO_s_mem())};
  ensureOrReturn(BIO_write(Bio.get(), Encoded.data(), Encoded.size()),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  ensureOrReturn(PEM_read_bio_PrivateKey(Bio.get(), &Sk, nullptr, nullptr),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return std::make_unique<KeyPair>(Sk);
}

template <uint32_t Nid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<Nid>::KeyPair>>
Ecdsa<Nid>::KeyPair::importPem(Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <uint32_t Nid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<Nid>::KeyPair>>
Ecdsa<Nid>::KeyPair::importRaw(Span<const uint8_t> Encoded) {
  auto InitCtx = initEC();
  if (!InitCtx) {
    return WasiCryptoUnexpect(InitCtx);
  }

  EVP_PKEY *Kp = *InitCtx;
  EC_KEY_set_conv_form(EVP_PKEY_get0_EC_KEY(Kp), POINT_CONVERSION_COMPRESSED);
  const uint8_t *Temp = Encoded.data();
  ensureOrReturn(Encoded.size() <= LONG_MAX,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  Kp = d2i_PrivateKey(EVP_PKEY_EC, &Kp, &Temp,
                      static_cast<long>(Encoded.size()));
  ensureOrReturn(Kp, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return std::make_unique<KeyPair>(Kp);
}

template <uint32_t Nid>
WasiCryptoExpect<std::vector<uint8_t>> Ecdsa<Nid>::KeyPair::exportPkcs8() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <uint32_t Nid>
WasiCryptoExpect<std::vector<uint8_t>> Ecdsa<Nid>::KeyPair::exportPem() {
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

template <uint32_t Nid>
WasiCryptoExpect<std::vector<uint8_t>> Ecdsa<Nid>::KeyPair::exportRaw() {
  EC_KEY_set_conv_form(EVP_PKEY_get0_EC_KEY(Ctx.get()),
                       POINT_CONVERSION_COMPRESSED);
  std::vector<uint8_t> Res(
      static_cast<size_t>(i2d_PrivateKey(Ctx.get(), nullptr)));
  uint8_t *Temp = Res.data();
  opensslAssuming(i2d_PrivateKey(Ctx.get(), &Temp));
  return Res;
}

template <uint32_t Nid>
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

template <uint32_t Nid>
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

template <uint32_t Nid>
WasiCryptoExpect<void> Ecdsa<Nid>::SignState::update(Span<const uint8_t> Data) {
  opensslAssuming(EVP_DigestSignUpdate(Ctx.get(), Data.data(), Data.size()));

  return {};
}

template <uint32_t Nid>
WasiCryptoExpect<std::unique_ptr<Signature>> Ecdsa<Nid>::SignState::sign() {
  size_t Size;
  opensslAssuming(EVP_DigestSignFinal(Ctx.get(), nullptr, &Size));

  std::vector<uint8_t> Res(Size);
  opensslAssuming(EVP_DigestSignFinal(Ctx.get(), Res.data(), &Size));

  return std::make_unique<Signature>(std::move(Res));
}

template <uint32_t Nid>
WasiCryptoExpect<void>
Ecdsa<Nid>::VerificationState::update(Span<const uint8_t> Data) {
  opensslAssuming(EVP_DigestVerifyUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

template <uint32_t Nid>
WasiCryptoExpect<void> Ecdsa<Nid>::VerificationState::verify(
    std::shared_ptr<Signatures::Signature> Sig) {
  ensureOrReturn(Sig->alg() == getAlg(), __WASI_CRYPTO_ERRNO_INVALID_SIGNATURE);

  opensslAssuming(
      EVP_DigestVerifyFinal(Ctx.get(), Sig->data().data(), Sig->data().size()));
  return {};
}

template <uint32_t Nid> WasiCryptoExpect<EVP_PKEY *> Ecdsa<Nid>::initEC() {
  EvpPkeyCtxPtr PCtx{EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr)};
  opensslAssuming(PCtx);
  opensslAssuming(EVP_PKEY_paramgen_init(PCtx.get()));
  opensslAssuming(EVP_PKEY_CTX_set_ec_paramgen_curve_nid(PCtx.get(), Nid));

  EVP_PKEY *Params = nullptr;
  opensslAssuming(EVP_PKEY_paramgen(PCtx.get(), &Params));
  return Params;
}

template class Ecdsa<NID_X9_62_prime256v1>;
template class Ecdsa<NID_secp256k1>;
} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
