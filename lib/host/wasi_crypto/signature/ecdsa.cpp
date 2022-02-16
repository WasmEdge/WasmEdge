// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/ecdsa.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/evpwrapper.h"
#include "host/wasi_crypto/signature/publickey.h"
#include "host/wasi_crypto/util.h"
#include "openssl/bio.h"
#include "openssl/ec.h"
#include "openssl/pem.h"
#include "openssl/x509.h"
#include "wasi_crypto/api.hpp"

#include <cstdint>
#include <cstdio>
#include <memory>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/ossl_typ.h>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {
namespace {
inline const size_t UnCompressedPkSize = 65;
inline const size_t CompressedPkSize = 33;
constexpr size_t getRawPkSize(bool Compressed) {
  return Compressed ? CompressedPkSize : UnCompressedPkSize;
}

inline const size_t SkSize = 32;

constexpr point_conversion_form_t getForm(bool Compressed) {
  return Compressed ? POINT_CONVERSION_COMPRESSED
                    : POINT_CONVERSION_UNCOMPRESSED;
}

WasiCryptoExpect<void> spanWriteToBio(BIO *Ptr, Span<const uint8_t> Span) {
  size_t Size;
  opensslAssuming(BIO_write_ex(Ptr, Span.data(), Span.size(), &Size));
  ensureOrReturn(Size == Span.size(), __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  return {};
}

WasiCryptoExpect<void> bioWriteToSpan(BIO *Ptr, Span<uint8_t> Span) {
  size_t Size;
  opensslAssuming(BIO_read_ex(Ptr, Span.data(), Span.size(), &Size));
  ensureOrReturn(Size == Span.size(), __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  return {};
}

template <int CurveNid> WasiCryptoExpect<void> checkEcSkIsValid(EVP_PKEY *Ctx) {
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  EC_KEY *EcCtx = EVP_PKEY_get0_EC_KEY(Ctx);
  ensureOrReturn(EcCtx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  const EC_GROUP *Group = EC_KEY_get0_group(EcCtx);
  ensureOrReturn(Group, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(EC_GROUP_get_curve_name(Group) == CurveNid,
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return {};
}

template <int CurveNid>
WasiCryptoExpect<void> checkEcPkIsValid(EVP_PKEY *Ctx, bool Compressed) {
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  EC_KEY *EcCtx = EVP_PKEY_get0_EC_KEY(Ctx);
  ensureOrReturn(EcCtx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  const EC_GROUP *Group = EC_KEY_get0_group(EcCtx);
  ensureOrReturn(Group, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(EC_GROUP_get_curve_name(Group) == CurveNid,
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(getForm(Compressed) ==
                     EC_GROUP_get_point_conversion_form(Group),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return {};
}

template <int CurveNid>
WasiCryptoExpect<void> checkEcKpIsValid(EVP_PKEY *Ctx, bool Compressed) {
  checkEcPkIsValid<CurveNid>(Ctx, Compressed);

  return {};
}

} // namespace
// raw secret scalar encoded as big endian, SEC-1, compressed SEC-1, unencrypted
// PKCS#8, PEM-encoded unencrypted PKCS#8

template <int CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::PublicKey>>
Ecdsa<CurveNid>::PublicKey::import(Span<const uint8_t> Encoded,
                                   __wasi_publickey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_PKCS8:
    return importPkcs8(Encoded, false);
  case __WASI_PUBLICKEY_ENCODING_COMPRESSED_PKCS8:
    return importPkcs8(Encoded, true);
  case __WASI_PUBLICKEY_ENCODING_PEM:
    return importPem(Encoded, false);
  case __WASI_PUBLICKEY_ENCODING_COMPRESSED_PEM:
    return importPem(Encoded, true);
  case __WASI_PUBLICKEY_ENCODING_SEC:
    return importSec(Encoded, false);
  case __WASI_PUBLICKEY_ENCODING_COMPRESSED_SEC:
    return importSec(Encoded, true);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <int CurveNid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<CurveNid>::PublicKey::exportData(__wasi_publickey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_SEC:
    return exportSec(false);
  case __WASI_PUBLICKEY_ENCODING_COMPRESSED_SEC:
    return exportSec(true);
  case __WASI_PUBLICKEY_ENCODING_PEM:
    return exportPem(false);
  case __WASI_PUBLICKEY_ENCODING_COMPRESSED_PEM:
    return exportPem(true);
  case __WASI_PUBLICKEY_ENCODING_PKCS8:
    return exportPkcs8(false);
  case __WASI_PUBLICKEY_ENCODING_COMPRESSED_PKCS8:
    return exportPkcs8(true);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <int CurveNid>
WasiCryptoExpect<std::unique_ptr<Signatures::VerificationState>>
Ecdsa<CurveNid>::PublicKey::openVerificationState() {
  EvpMdCtxPtr SignCtx{EVP_MD_CTX_create()};
  opensslAssuming(EVP_DigestVerifyInit(SignCtx.get(), nullptr, EVP_sha256(),
                                       nullptr, Ctx.get()));
  return std::make_unique<VerificationState>(std::move(SignCtx));
}

template <int CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::PublicKey>>
Ecdsa<CurveNid>::PublicKey::importPkcs8(Span<const uint8_t> Encoded,
                                        bool Compressed) {
  EvpPkeyPtr Ctx{
      d2i_PUBKEY(nullptr, addressOfTempory(Encoded.data()), Encoded.size())};
  opensslAssuming(Ctx);
  // ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  auto Res = checkEcPkIsValid<CurveNid>(Ctx.get(), Compressed);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return std::make_unique<PublicKey>(std::move(Ctx));
}

template <int CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::PublicKey>>
Ecdsa<CurveNid>::PublicKey::importPem(Span<const uint8_t> Encoded,
                                      bool Compressed) {
  BioPtr Bio{BIO_new(BIO_s_mem())};

  auto WriteRes = spanWriteToBio(Bio.get(), Encoded);
  if (!WriteRes) {
    return WasiCryptoUnexpect(WriteRes);
  }

  EvpPkeyPtr Ctx{PEM_read_bio_PUBKEY(Bio.get(), nullptr, nullptr, nullptr)};

  auto CheckResult = checkEcPkIsValid<CurveNid>(Ctx.get(), Compressed);
  if (!CheckResult) {
    return WasiCryptoUnexpect(CheckResult);
  }

  return std::make_unique<PublicKey>(std::move(Ctx));
}

template <int CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::PublicKey>>
Ecdsa<CurveNid>::PublicKey::importSec(Span<const uint8_t> Encoded,
                                      bool Compressed) {
  ensureOrReturn(Encoded.size() == getRawPkSize(Compressed),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);

  EcKeyPtr EcCtx{EC_KEY_new_by_curve_name(CurveNid)};
  EcPointPtr Pk{EC_POINT_new(EC_KEY_get0_group(EcCtx.get()))};
  opensslAssuming(EC_POINT_oct2point(EC_KEY_get0_group(EcCtx.get()), Pk.get(),
                                     Encoded.data(), Encoded.size(), nullptr));
  opensslAssuming(EC_KEY_set_public_key(EcCtx.get(), Pk.get()));

  EvpPkeyPtr Ctx{EVP_PKEY_new()};
  opensslAssuming(EVP_PKEY_set1_EC_KEY(Ctx.get(), EcCtx.get()));
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return std::make_unique<PublicKey>(std::move(Ctx));
}

template <int CurveNid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<CurveNid>::PublicKey::exportSec(bool Compressed) {
  EC_KEY *EcCtx = EVP_PKEY_get0_EC_KEY(Ctx.get());
  std::vector<uint8_t> Res(getRawPkSize(Compressed));
  opensslAssuming(EC_POINT_point2oct(
      EC_KEY_get0_group(EcCtx), EC_KEY_get0_public_key(EcCtx),
      getForm(Compressed), Res.data(), Res.size(), nullptr));
  return Res;
}

template <int CurveNid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<CurveNid>::PublicKey::exportPem(bool Compressed) {
  EC_KEY_set_conv_form(EVP_PKEY_get0_EC_KEY(Ctx.get()), getForm(Compressed));

  BioPtr Bio{BIO_new(BIO_s_mem())};
  opensslAssuming(PEM_write_bio_PUBKEY(Bio.get(), Ctx.get()));
  std::vector<uint8_t> Res(
      static_cast<size_t>(BIO_get_mem_data(Bio.get(), nullptr)));
  bioWriteToSpan(Bio.get(), Res);
  return Res;
}

template <int CurveNid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<CurveNid>::PublicKey::exportPkcs8(bool Compressed) {
  EC_KEY_set_conv_form(EVP_PKEY_get0_EC_KEY(Ctx.get()), getForm(Compressed));

  std::vector<uint8_t> Res(static_cast<size_t>(i2d_PUBKEY(Ctx.get(), nullptr)));
  opensslAssuming(i2d_PUBKEY(Ctx.get(), addressOfTempory(Res.data())));
  return Res;
}

template <int CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::SecretKey>>
Ecdsa<CurveNid>::SecretKey::import(Span<const uint8_t> Encoded,
                                   __wasi_secretkey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW:
    return importRaw(Encoded);
  case __WASI_SECRETKEY_ENCODING_PKCS8:
    return importPkcs8(Encoded);
  case __WASI_SECRETKEY_ENCODING_PEM:
    return importPem(Encoded);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <int CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::SecretKey>>
Ecdsa<CurveNid>::SecretKey::importPkcs8(Span<const uint8_t> Encoded) {
  // BioPtr Bio{BIO_new(BIO_s_mem())};

  // auto WriteRes = spanWriteToBio(Bio.get(), Encoded);
  // if (!WriteRes) {
  //   return WasiCryptoUnexpect(WriteRes);
  // }

  EvpPkeyPtr Ctx{d2i_PrivateKey(EVP_PKEY_EC, nullptr,
                                addressOfTempory(Encoded.data()),
                                static_cast<long>(Encoded.size()))};
  auto CheckResult = checkEcSkIsValid<CurveNid>(Ctx.get());
  if (!CheckResult) {
    return WasiCryptoUnexpect(CheckResult);
  }

  return std::make_unique<SecretKey>(std::move(Ctx));
}

template <int CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::SecretKey>>
Ecdsa<CurveNid>::SecretKey::importPem(Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};

  auto WriteRes = spanWriteToBio(Bio.get(), Encoded);
  if (!WriteRes) {
    return WasiCryptoUnexpect(WriteRes);
  }

  EvpPkeyPtr Ctx{PEM_read_bio_PrivateKey(Bio.get(), nullptr, nullptr, nullptr)};
  auto CheckResult = checkEcSkIsValid<CurveNid>(Ctx.get());
  if (!CheckResult) {
    return WasiCryptoUnexpect(CheckResult);
  }

  return std::make_unique<SecretKey>(std::move(Ctx));
}

template <int CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::SecretKey>>
Ecdsa<CurveNid>::SecretKey::importRaw(Span<const uint8_t> Encoded) {
  EcKeyPtr EcCtx{EC_KEY_new_by_curve_name(CurveNid)};
  BnPtr Sk{
      BN_bin2bn(Encoded.data(), static_cast<int>(Encoded.size()), nullptr)};
  ensureOrReturn(EC_KEY_set_private_key(EcCtx.get(), Sk.get()),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);

  // caculate Pk and set
  EcPointPtr Pk{EC_POINT_new(EC_KEY_get0_group(EcCtx.get()))};
  opensslAssuming(EC_POINT_mul(EC_KEY_get0_group(EcCtx.get()), Pk.get(),
                               Sk.get(), nullptr, nullptr, nullptr));
  opensslAssuming(EC_KEY_set_public_key(EcCtx.get(), Pk.get()));

  EvpPkeyPtr Ctx{EVP_PKEY_new()};
  opensslAssuming(EVP_PKEY_set1_EC_KEY(Ctx.get(), EcCtx.get()));
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return std::make_unique<SecretKey>(std::move(Ctx));
}

template <int CurveNid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<CurveNid>::SecretKey::exportData(__wasi_secretkey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW:
    return exportRaw();
  case __WASI_SECRETKEY_ENCODING_PKCS8:
    return exportPkcs8();
  case __WASI_SECRETKEY_ENCODING_PEM:
    return exportPem();
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}
template <int CurveNid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<CurveNid>::SecretKey::exportPkcs8() {
  std::vector<uint8_t> Res(
      static_cast<size_t>(i2d_PrivateKey(Ctx.get(), nullptr)));
  opensslAssuming(i2d_PrivateKey(Ctx.get(), addressOfTempory(Res.data())));
  return Res;
}

template <int CurveNid>
WasiCryptoExpect<std::vector<uint8_t>> Ecdsa<CurveNid>::SecretKey::exportPem() {
  BioPtr Bio{BIO_new(BIO_s_mem())};
  ensureOrReturn(PEM_write_bio_PrivateKey(Bio.get(), Ctx.get(), nullptr,
                                          nullptr, 0, nullptr, nullptr),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  std::vector<uint8_t> Pem(
      static_cast<size_t>(BIO_get_mem_data(Bio.get(), nullptr)));
  auto Res = bioWriteToSpan(Bio.get(), Pem);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return Pem;
}

template <int CurveNid>
WasiCryptoExpect<std::vector<uint8_t>> Ecdsa<CurveNid>::SecretKey::exportRaw() {
  const BIGNUM *Sk = EC_KEY_get0_private_key(EVP_PKEY_get0_EC_KEY(Ctx.get()));
  std::vector<uint8_t> Res(SkSize);
  opensslAssuming(BN_bn2bin(Sk, Res.data()));

  return Res;
}

template <int CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::KeyPair>>
Ecdsa<CurveNid>::KeyPair::generate(std::shared_ptr<Options>) {
  auto InitCtx = initEC();

  // Generate Key
  EvpPkeyCtxPtr KCtx{EVP_PKEY_CTX_new(InitCtx.get(), nullptr)};
  opensslAssuming(KCtx);
  opensslAssuming(EVP_PKEY_keygen_init(KCtx.get()));

  EVP_PKEY *Key = nullptr;
  opensslAssuming(EVP_PKEY_keygen(KCtx.get(), &Key));

  return std::make_unique<KeyPair>(EvpPkeyPtr{Key});
}

template <int CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::KeyPair>>
Ecdsa<CurveNid>::KeyPair::import(Span<const uint8_t> Encoded,
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

template <int CurveNid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<CurveNid>::KeyPair::exportData(__wasi_keypair_encoding_e_t Encoding) {
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

template <int CurveNid>
WasiCryptoExpect<std::unique_ptr<Signatures::PublicKey>>
Ecdsa<CurveNid>::KeyPair::publicKey() {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PUBKEY_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PUBKEY_bio(B.get(), &Res));

  return std::make_unique<PublicKey>(EvpPkeyPtr{Res});
}

template <int CurveNid>
WasiCryptoExpect<std::unique_ptr<SecretKey>>
Ecdsa<CurveNid>::KeyPair::secretKey() {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PrivateKey_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PrivateKey_bio(B.get(), &Res));

  return std::make_unique<SecretKey>(EvpPkeyPtr{Res});
}

template <int CurveNid>
WasiCryptoExpect<std::unique_ptr<SignState>>
Ecdsa<CurveNid>::KeyPair::openSignState() {
  EvpMdCtxPtr SignCtx{EVP_MD_CTX_create()};
  opensslAssuming(EVP_DigestSignInit(SignCtx.get(), nullptr, EVP_sha256(),
                                     nullptr, Ctx.get()));

  return std::make_unique<SignState>(std::move(SignCtx));
}

template <int CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::KeyPair>>
Ecdsa<CurveNid>::KeyPair::importPkcs8(Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};

  auto WriteRes = spanWriteToBio(Bio.get(), Encoded);
  if (!WriteRes) {
    return WasiCryptoUnexpect(WriteRes);
  }

  EvpPkeyPtr Ctx{d2i_PrivateKey_bio(Bio.get(), nullptr)};
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return std::make_unique<KeyPair>(std::move(Ctx));
}

template <int CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::KeyPair>>
Ecdsa<CurveNid>::KeyPair::importPem(Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};

  auto WriteRes = spanWriteToBio(Bio.get(), Encoded);
  if (!WriteRes) {
    return WasiCryptoUnexpect(WriteRes);
  }

  EvpPkeyPtr Ctx{PEM_read_bio_PrivateKey(Bio.get(), nullptr, nullptr, nullptr)};
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return std::make_unique<KeyPair>(std::move(Ctx));
}

template <int CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::KeyPair>>
Ecdsa<CurveNid>::KeyPair::importRaw(Span<const uint8_t> Encoded) {
  EcKeyPtr EcCtx{EC_KEY_new_by_curve_name(CurveNid)};
  BnPtr Sk{
      BN_bin2bn(Encoded.data(), static_cast<int>(Encoded.size()), nullptr)};
  ensureOrReturn(EC_KEY_set_private_key(EcCtx.get(), Sk.get()),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);

  // caculate Pk and set
  EcPointPtr Pk{EC_POINT_new(EC_KEY_get0_group(EcCtx.get()))};
  opensslAssuming(EC_POINT_mul(EC_KEY_get0_group(EcCtx.get()), Pk.get(),
                               Sk.get(), nullptr, nullptr, nullptr));
  opensslAssuming(EC_KEY_set_public_key(EcCtx.get(), Pk.get()));

  EvpPkeyPtr Ctx{EVP_PKEY_new()};
  opensslAssuming(EVP_PKEY_set1_EC_KEY(Ctx.get(), EcCtx.get()));
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return std::make_unique<KeyPair>(std::move(Ctx));
}

template <int CurveNid>
WasiCryptoExpect<std::vector<uint8_t>> Ecdsa<CurveNid>::KeyPair::exportPkcs8() {
  std::vector<uint8_t> Res(
      static_cast<size_t>(i2d_PrivateKey(Ctx.get(), nullptr)));
  opensslAssuming(i2d_PrivateKey(Ctx.get(), addressOfTempory(Res.data())));
  return Res;
}

template <int CurveNid>
WasiCryptoExpect<std::vector<uint8_t>> Ecdsa<CurveNid>::KeyPair::exportPem() {
  BioPtr Bio{BIO_new(BIO_s_mem())};
  ensureOrReturn(PEM_write_bio_PrivateKey(Bio.get(), Ctx.get(), nullptr,
                                          nullptr, 0, nullptr, nullptr),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  std::vector<uint8_t> Pem(
      static_cast<size_t>(BIO_get_mem_data(Bio.get(), nullptr)));

  auto Res = bioWriteToSpan(Bio.get(), Pem);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return Pem;
}

template <int CurveNid>
WasiCryptoExpect<std::vector<uint8_t>> Ecdsa<CurveNid>::KeyPair::exportRaw() {
  std::vector<uint8_t> Res(
      static_cast<size_t>(i2d_PrivateKey(Ctx.get(), nullptr)));
  opensslAssuming(i2d_PrivateKey(Ctx.get(), addressOfTempory(Res.data())));
  return Res;
}

template <int CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::Signature>>
Ecdsa<CurveNid>::Signature::import(Span<const uint8_t> Encoded,
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

template <int CurveNid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<CurveNid>::Signature::exportData(__wasi_signature_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW:
    return Data;
  case __WASI_SIGNATURE_ENCODING_DER:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  default:
    assumingUnreachable();
  }
}

template <int CurveNid>
WasiCryptoExpect<void>
Ecdsa<CurveNid>::SignState::update(Span<const uint8_t> Data) {
  opensslAssuming(EVP_DigestSignUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

template <int CurveNid>
WasiCryptoExpect<std::unique_ptr<Signature>>
Ecdsa<CurveNid>::SignState::sign() {
  size_t Size;
  opensslAssuming(EVP_DigestSignFinal(Ctx.get(), nullptr, &Size));

  std::vector<uint8_t> Res(Size);
  opensslAssuming(EVP_DigestSignFinal(Ctx.get(), Res.data(), &Size));

  return std::make_unique<Signature>(std::move(Res));
}

template <int CurveNid>
WasiCryptoExpect<void>
Ecdsa<CurveNid>::VerificationState::update(Span<const uint8_t> Data) {
  opensslAssuming(EVP_DigestVerifyUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

template <int CurveNid>
WasiCryptoExpect<void> Ecdsa<CurveNid>::VerificationState::verify(
    std::shared_ptr<Signatures::Signature> Sig) {
  ensureOrReturn(Sig->alg() == getAlg(), __WASI_CRYPTO_ERRNO_INVALID_SIGNATURE);

  ensureOrReturn(
      EVP_DigestVerifyFinal(Ctx.get(), Sig->data().data(), Sig->data().size()),
      __WASI_CRYPTO_ERRNO_VERIFICATION_FAILED);
  return {};
}

template <int CurveNid> EvpPkeyPtr Ecdsa<CurveNid>::initEC() {
  EvpPkeyCtxPtr PCtx{EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr)};
  EVP_PKEY_paramgen_init(PCtx.get());
  EVP_PKEY_CTX_set_ec_paramgen_curve_nid(PCtx.get(), CurveNid);

  EVP_PKEY *Params = nullptr;
  EVP_PKEY_paramgen(PCtx.get(), &Params);
  return EvpPkeyPtr{Params};
}

template class Ecdsa<NID_X9_62_prime256v1>;
template class Ecdsa<NID_secp256k1>;
} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
