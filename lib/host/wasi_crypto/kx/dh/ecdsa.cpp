// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/kx/dh/ecdsa.h"
#include "host/wasi_crypto/utils/error.h"
#include "host/wasi_crypto/utils/evp_wrapper.h"
#include "host/wasi_crypto/utils/optional.h"
#include "openssl/pem.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

namespace {
inline const size_t UnCompressedPkSize = 65;
inline const size_t CompressedPkSize = 33;
constexpr size_t getRawPkSize(bool Compressed) {
  return Compressed ? CompressedPkSize : UnCompressedPkSize;
}

inline const size_t SkSize = 32;
inline const size_t SharedSecretSize = 32;

constexpr point_conversion_form_t getForm(bool Compressed) noexcept {
  return Compressed ? POINT_CONVERSION_COMPRESSED
                    : POINT_CONVERSION_UNCOMPRESSED;
}

} // namespace
// raw secret scalar encoded as big endian, SEC-1, compressed SEC-1, unencrypted
// PKCS#8, PEM-encoded unencrypted PKCS#8

WasiCryptoExpect<Ecdsa::PublicKey>
Ecdsa::PublicKey::import(Span<const uint8_t> Encoded,
                         __wasi_publickey_encoding_e_t Encoding) noexcept {
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

WasiCryptoExpect<std::vector<uint8_t>> Ecdsa::PublicKey::exportData(
    __wasi_publickey_encoding_e_t Encoding) const noexcept {
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

WasiCryptoExpect<void> Ecdsa::PublicKey::verify() const noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<EvpPkeyPtr> Ecdsa::PublicKey::checkValid(EvpPkeyPtr Ctx,
                                                          bool) noexcept {
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  EC_KEY *EcCtx = EVP_PKEY_get0_EC_KEY(Ctx.get());
  ensureOrReturn(EcCtx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  const EC_GROUP *Group = EC_KEY_get0_group(EcCtx);
  ensureOrReturn(Group, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(EC_GROUP_get_curve_name(Group) == NID_X9_62_prime256v1,
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return {std::move(Ctx)};
}

WasiCryptoExpect<Ecdsa::PublicKey>
Ecdsa::PublicKey::importPkcs8(Span<const uint8_t> Encoded,
                              bool Compressed) noexcept {
  return checkValid(EvpPkeyPtr{d2iPUBKEY(Encoded)}, Compressed);
}

WasiCryptoExpect<Ecdsa::PublicKey>
Ecdsa::PublicKey::importPem(Span<const uint8_t> Encoded,
                            bool Compressed) noexcept {
  return checkValid(EvpPkeyPtr{pemReadPUBKEY(Encoded)}, Compressed);
}

WasiCryptoExpect<Ecdsa::PublicKey>
Ecdsa::PublicKey::importSec(Span<const uint8_t> Encoded,
                            bool Compressed) noexcept {
  EcKeyPtr EcCtx{EC_KEY_new_by_curve_name(NID_X9_62_prime256v1)};
  EcPointPtr Pk{EC_POINT_new(EC_KEY_get0_group(EcCtx.get()))};
  ensureOrReturn(EC_POINT_oct2point(EC_KEY_get0_group(EcCtx.get()), Pk.get(),
                                    Encoded.data(), Encoded.size(), nullptr),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  opensslCheck(EC_KEY_set_public_key(EcCtx.get(), Pk.get()));

  EvpPkeyPtr Ctx{EVP_PKEY_new()};
  opensslCheck(EVP_PKEY_set1_EC_KEY(Ctx.get(), EcCtx.get()));

  return checkValid(std::move(Ctx), Compressed);
}

WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa::PublicKey::exportSec(bool Compressed) const noexcept {
  EC_KEY *EcCtx = EVP_PKEY_get0_EC_KEY(Ctx.get());
  std::vector<uint8_t> Res(getRawPkSize(Compressed));
  opensslCheck(EC_POINT_point2oct(
      EC_KEY_get0_group(EcCtx), EC_KEY_get0_public_key(EcCtx),
      getForm(Compressed), Res.data(), Res.size(), nullptr));
  return Res;
}

WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa::PublicKey::exportPem(bool Compressed) const noexcept {
  EC_KEY_set_conv_form(EVP_PKEY_get0_EC_KEY(Ctx.get()), getForm(Compressed));

  return pemWritePUBKEY(Ctx.get());
}

WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa::PublicKey::exportPkcs8(bool Compressed) const noexcept {
  EC_KEY_set_conv_form(EVP_PKEY_get0_EC_KEY(Ctx.get()), getForm(Compressed));

  return i2dPUBKEY(Ctx.get());
}

WasiCryptoExpect<Ecdsa::SecretKey>
Ecdsa::SecretKey::import(Span<const uint8_t> Encoded,
                         __wasi_secretkey_encoding_e_t Encoding) noexcept {
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

WasiCryptoExpect<EvpPkeyPtr>
Ecdsa::SecretKey::checkValid(EvpPkeyPtr Ctx) noexcept {
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  EC_KEY *EcCtx = EVP_PKEY_get0_EC_KEY(Ctx.get());
  ensureOrReturn(EcCtx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  const EC_GROUP *Group = EC_KEY_get0_group(EcCtx);
  ensureOrReturn(Group, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(EC_GROUP_get_curve_name(Group) == NID_X9_62_prime256v1,
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return {std::move(Ctx)};
}

WasiCryptoExpect<Ecdsa::SecretKey>
Ecdsa::SecretKey::importPkcs8(Span<const uint8_t> Encoded) noexcept {
  return checkValid(EvpPkeyPtr{d2iPrivateKey(Encoded)});
}

WasiCryptoExpect<Ecdsa::SecretKey>
Ecdsa::SecretKey::importPem(Span<const uint8_t> Encoded) noexcept {
  return checkValid(EvpPkeyPtr{pemReadPrivateKey(Encoded)});
}

WasiCryptoExpect<Ecdsa::SecretKey>
Ecdsa::SecretKey::importRaw(Span<const uint8_t> Encoded) noexcept {
  EcKeyPtr EcCtx{EC_KEY_new_by_curve_name(NID_X9_62_prime256v1)};
  ensureOrReturn(Encoded.size() == SkSize, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  BnPtr Sk{
      BN_bin2bn(Encoded.data(), static_cast<int>(Encoded.size()), nullptr)};
  ensureOrReturn(EC_KEY_set_private_key(EcCtx.get(), Sk.get()),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);

  EvpPkeyPtr Ctx{EVP_PKEY_new()};
  opensslCheck(EVP_PKEY_set1_EC_KEY(Ctx.get(), EcCtx.get()));

  return Ctx;
}

WasiCryptoExpect<Ecdsa::KeyPair>
Ecdsa::SecretKey::toKeyPair(const PublicKey &) const noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<SecretVec> Ecdsa::SecretKey::exportData(
    __wasi_secretkey_encoding_e_t Encoding) const noexcept {
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

WasiCryptoExpect<SecretVec> Ecdsa::SecretKey::exportPkcs8() const noexcept {
  return i2dPrivateKey(Ctx.get());
}

WasiCryptoExpect<SecretVec> Ecdsa::SecretKey::exportPem() const noexcept {
  return pemWritePrivateKey(Ctx.get());
}

WasiCryptoExpect<SecretVec> Ecdsa::SecretKey::exportRaw() const noexcept {
  // must equal to SkSize, not check.
  const BIGNUM *Sk = EC_KEY_get0_private_key(EVP_PKEY_get0_EC_KEY(Ctx.get()));
  SecretVec Res(SkSize);
  opensslCheck(BN_bn2bin(Sk, Res.data()));

  return Res;
}

WasiCryptoExpect<Ecdsa::PublicKey>
Ecdsa::SecretKey::publicKey() const noexcept {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslCheck(i2d_PUBKEY_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslCheck(d2i_PUBKEY_bio(B.get(), &Res));

  return EvpPkeyPtr{Res};
}

WasiCryptoExpect<SecretVec>
Ecdsa::SecretKey::dh(const PublicKey &Pk) const noexcept {
  EvpPkeyCtxPtr SkCtx{EVP_PKEY_CTX_new(Ctx.get(), nullptr)};
  opensslCheck(EVP_PKEY_derive_init(SkCtx.get()));

  // set peer key
  opensslCheck(EVP_PKEY_derive_set_peer(SkCtx.get(), Pk.raw().get()));

  // generate shared secret
  SecretVec Res(SharedSecretSize);
  size_t Size = SharedSecretSize;
  ensureOrReturn(EVP_PKEY_derive(SkCtx.get(), Res.data(), &Size),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(Size == SharedSecretSize,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return Res;
}

WasiCryptoExpect<Ecdsa::KeyPair>
Ecdsa::KeyPair::generate(OptionalRef<const Options>) noexcept {
  EvpPkeyCtxPtr ParamCtx{EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr)};
  EVP_PKEY_keygen_init(ParamCtx.get());
  EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ParamCtx.get(), NID_X9_62_prime256v1);

  EVP_PKEY *Key = nullptr;
  opensslCheck(EVP_PKEY_keygen(ParamCtx.get(), &Key));

  return EvpPkeyPtr{Key};
}

WasiCryptoExpect<EvpPkeyPtr> Ecdsa::KeyPair::checkValid(EvpPkeyPtr Ctx,
                                                        bool) noexcept {
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  EC_KEY *EcCtx = EVP_PKEY_get0_EC_KEY(Ctx.get());
  ensureOrReturn(EcCtx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  // curve id check
  const EC_GROUP *Group = EC_KEY_get0_group(EcCtx);
  ensureOrReturn(Group, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(EC_GROUP_get_curve_name(Group) == NID_X9_62_prime256v1,
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  // have public key
  ensureOrReturn(EC_KEY_get0_public_key(EcCtx),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return {std::move(Ctx)};
}

WasiCryptoExpect<Ecdsa::KeyPair>
Ecdsa::KeyPair::import(Span<const uint8_t> Encoded,
                       __wasi_keypair_encoding_e_t Encoding) noexcept {
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW:
    return importRaw(Encoded);
  case __WASI_KEYPAIR_ENCODING_PKCS8:
    return importPkcs8(Encoded, false);
  case __WASI_KEYPAIR_ENCODING_PEM:
    return importPem(Encoded, false);
  case __WASI_KEYPAIR_ENCODING_COMPRESSED_PKCS8:
    return importPkcs8(Encoded, true);
  case __WASI_KEYPAIR_ENCODING_COMPRESSED_PEM:
    return importPem(Encoded, true);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<SecretVec> Ecdsa::KeyPair::exportData(
    __wasi_keypair_encoding_e_t Encoding) const noexcept {
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

WasiCryptoExpect<Ecdsa::PublicKey> Ecdsa::KeyPair::publicKey() const noexcept {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslCheck(i2d_PUBKEY_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslCheck(d2i_PUBKEY_bio(B.get(), &Res));

  return EvpPkeyPtr{Res};
}

WasiCryptoExpect<Ecdsa::SecretKey> Ecdsa::KeyPair::secretKey() const noexcept {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslCheck(i2d_PrivateKey_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslCheck(d2i_PrivateKey_bio(B.get(), &Res));

  return EvpPkeyPtr{Res};
}

WasiCryptoExpect<Ecdsa::KeyPair>
Ecdsa::KeyPair::importPkcs8(Span<const uint8_t> Encoded,
                            bool Compressed) noexcept {
  return checkValid(EvpPkeyPtr{d2iPrivateKey(Encoded)}, Compressed);
}

WasiCryptoExpect<Ecdsa::KeyPair>
Ecdsa::KeyPair::importPem(Span<const uint8_t> Encoded,
                          bool Compressed) noexcept {
  return checkValid(EvpPkeyPtr{pemReadPrivateKey(Encoded)}, Compressed);
}

WasiCryptoExpect<Ecdsa::KeyPair>
Ecdsa::KeyPair::importRaw(Span<const uint8_t> Encoded) noexcept {
  ensureOrReturn(Encoded.size() == SkSize, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  EcKeyPtr EcCtx{EC_KEY_new_by_curve_name(NID_X9_62_prime256v1)};
  BnPtr Sk{
      BN_bin2bn(Encoded.data(), static_cast<int>(Encoded.size()), nullptr)};
  ensureOrReturn(EC_KEY_set_private_key(EcCtx.get(), Sk.get()),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);

  // calculate Pk and set
  EcPointPtr Pk{EC_POINT_new(EC_KEY_get0_group(EcCtx.get()))};
  opensslCheck(EC_POINT_mul(EC_KEY_get0_group(EcCtx.get()), Pk.get(), Sk.get(),
                            nullptr, nullptr, nullptr));
  opensslCheck(EC_KEY_set_public_key(EcCtx.get(), Pk.get()));

  EvpPkeyPtr Ctx{EVP_PKEY_new()};
  opensslCheck(EVP_PKEY_set1_EC_KEY(Ctx.get(), EcCtx.get()));
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return Ctx;
}

WasiCryptoExpect<SecretVec> Ecdsa::KeyPair::exportPkcs8() const noexcept {
  return i2dPrivateKey(Ctx.get());
}

WasiCryptoExpect<SecretVec> Ecdsa::KeyPair::exportPem() const noexcept {
  return pemWritePrivateKey(Ctx.get());
}

WasiCryptoExpect<SecretVec> Ecdsa::KeyPair::exportRaw() const noexcept {
  const BIGNUM *Sk = EC_KEY_get0_private_key(EVP_PKEY_get0_EC_KEY(Ctx.get()));
  SecretVec Res(SkSize);
  opensslCheck(BN_bn2bin(Sk, Res.data()));

  return Res;
}


} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
