// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/signatures/ecdsa.h"
#include "host/wasi_crypto/utils/error.h"
#include "host/wasi_crypto/utils/evp_wrapper.h"
#include "host/wasi_crypto/utils/optional.h"
#include "openssl/pem.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

namespace {
inline const size_t UnCompressedPkSize = 65;
inline const size_t CompressedPkSize = 33;
constexpr size_t getRawPkSize(bool Compressed) {
  return Compressed ? CompressedPkSize : UnCompressedPkSize;
}

inline const size_t SkSize = 32;
inline const size_t RawSigSize = 64;

constexpr point_conversion_form_t getForm(bool Compressed) noexcept {
  return Compressed ? POINT_CONVERSION_COMPRESSED
                    : POINT_CONVERSION_UNCOMPRESSED;
}

} // namespace
// raw secret scalar encoded as big endian, SEC-1, compressed SEC-1, unencrypted
// PKCS#8, PEM-encoded unencrypted PKCS#8

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::PublicKey>
Ecdsa<CurveNid>::PublicKey::import(
    Span<const uint8_t> Encoded,
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

template <int CurveNid>
WasiCryptoExpect<std::vector<uint8_t>> Ecdsa<CurveNid>::PublicKey::exportData(
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

template <int CurveNid>
WasiCryptoExpect<void> Ecdsa<CurveNid>::PublicKey::verify() const noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::VerificationState>
Ecdsa<CurveNid>::PublicKey::openVerificationState() const noexcept {
  EvpMdCtxPtr SignCtx{EVP_MD_CTX_create()};
  opensslCheck(EVP_DigestVerifyInit(SignCtx.get(), nullptr, EVP_sha256(),
                                    nullptr, Ctx.get()));
  return SignCtx;
}

template <int CurveNid>
WasiCryptoExpect<EvpPkeyPtr>
Ecdsa<CurveNid>::PublicKey::checkValid(EvpPkeyPtr Ctx, bool) noexcept {
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  EC_KEY *EcCtx = EVP_PKEY_get0_EC_KEY(Ctx.get());
  ensureOrReturn(EcCtx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  const EC_GROUP *Group = EC_KEY_get0_group(EcCtx);
  ensureOrReturn(Group, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(EC_GROUP_get_curve_name(Group) == CurveNid,
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return {std::move(Ctx)};
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::PublicKey>
Ecdsa<CurveNid>::PublicKey::importPkcs8(Span<const uint8_t> Encoded,
                                        bool Compressed) noexcept {
  return checkValid(EvpPkeyPtr{d2iPUBKEY(Encoded)}, Compressed);
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::PublicKey>
Ecdsa<CurveNid>::PublicKey::importPem(Span<const uint8_t> Encoded,
                                      bool Compressed) noexcept {
  return checkValid(EvpPkeyPtr{pemReadPUBKEY(Encoded)}, Compressed);
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::PublicKey>
Ecdsa<CurveNid>::PublicKey::importSec(Span<const uint8_t> Encoded,
                                      bool Compressed) noexcept {
  EcKeyPtr EcCtx{EC_KEY_new_by_curve_name(CurveNid)};
  EcPointPtr Pk{EC_POINT_new(EC_KEY_get0_group(EcCtx.get()))};
  ensureOrReturn(EC_POINT_oct2point(EC_KEY_get0_group(EcCtx.get()), Pk.get(),
                                    Encoded.data(), Encoded.size(), nullptr),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  opensslCheck(EC_KEY_set_public_key(EcCtx.get(), Pk.get()));

  EvpPkeyPtr Ctx{EVP_PKEY_new()};
  opensslCheck(EVP_PKEY_set1_EC_KEY(Ctx.get(), EcCtx.get()));

  return checkValid(std::move(Ctx), Compressed);
}

template <int CurveNid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<CurveNid>::PublicKey::exportSec(bool Compressed) const noexcept {
  EC_KEY *EcCtx = EVP_PKEY_get0_EC_KEY(Ctx.get());
  std::vector<uint8_t> Res(getRawPkSize(Compressed));
  opensslCheck(EC_POINT_point2oct(
      EC_KEY_get0_group(EcCtx), EC_KEY_get0_public_key(EcCtx),
      getForm(Compressed), Res.data(), Res.size(), nullptr));
  return Res;
}

template <int CurveNid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<CurveNid>::PublicKey::exportPem(bool Compressed) const noexcept {
  EC_KEY_set_conv_form(EVP_PKEY_get0_EC_KEY(Ctx.get()), getForm(Compressed));

  return pemWritePUBKEY(Ctx.get());
}

template <int CurveNid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<CurveNid>::PublicKey::exportPkcs8(bool Compressed) const noexcept {
  EC_KEY_set_conv_form(EVP_PKEY_get0_EC_KEY(Ctx.get()), getForm(Compressed));

  return i2dPUBKEY(Ctx.get());
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::SecretKey>
Ecdsa<CurveNid>::SecretKey::import(
    Span<const uint8_t> Encoded,
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

template <int CurveNid>
WasiCryptoExpect<EvpPkeyPtr>
Ecdsa<CurveNid>::SecretKey::checkValid(EvpPkeyPtr Ctx) noexcept {
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  EC_KEY *EcCtx = EVP_PKEY_get0_EC_KEY(Ctx.get());
  ensureOrReturn(EcCtx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  const EC_GROUP *Group = EC_KEY_get0_group(EcCtx);
  ensureOrReturn(Group, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(EC_GROUP_get_curve_name(Group) == CurveNid,
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return {std::move(Ctx)};
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::SecretKey>
Ecdsa<CurveNid>::SecretKey::importPkcs8(Span<const uint8_t> Encoded) noexcept {
  return checkValid(EvpPkeyPtr{d2iPrivateKey(Encoded)});
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::SecretKey>
Ecdsa<CurveNid>::SecretKey::importPem(Span<const uint8_t> Encoded) noexcept {
  return checkValid(EvpPkeyPtr{pemReadPrivateKey(Encoded)});
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::SecretKey>
Ecdsa<CurveNid>::SecretKey::importRaw(Span<const uint8_t> Encoded) noexcept {
  EcKeyPtr EcCtx{EC_KEY_new_by_curve_name(CurveNid)};
  ensureOrReturn(Encoded.size() == SkSize, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  BnPtr Sk{
      BN_bin2bn(Encoded.data(), static_cast<int>(Encoded.size()), nullptr)};
  ensureOrReturn(EC_KEY_set_private_key(EcCtx.get(), Sk.get()),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);

  EvpPkeyPtr Ctx{EVP_PKEY_new()};
  opensslCheck(EVP_PKEY_set1_EC_KEY(Ctx.get(), EcCtx.get()));

  return Ctx;
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::KeyPair>
Ecdsa<CurveNid>::SecretKey::toKeyPair(const PublicKey &) const noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int CurveNid>
WasiCryptoExpect<SecretVec> Ecdsa<CurveNid>::SecretKey::exportData(
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

template <int CurveNid>
WasiCryptoExpect<SecretVec>
Ecdsa<CurveNid>::SecretKey::exportPkcs8() const noexcept {
  return i2dPrivateKey(Ctx.get());
}

template <int CurveNid>
WasiCryptoExpect<SecretVec>
Ecdsa<CurveNid>::SecretKey::exportPem() const noexcept {
  return pemWritePrivateKey(Ctx.get());
}

template <int CurveNid>
WasiCryptoExpect<SecretVec>
Ecdsa<CurveNid>::SecretKey::exportRaw() const noexcept {
  // must equal to SkSize, not check.
  const BIGNUM *Sk = EC_KEY_get0_private_key(EVP_PKEY_get0_EC_KEY(Ctx.get()));
  SecretVec Res(SkSize);
  opensslCheck(BN_bn2bin(Sk, Res.data()));

  return Res;
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::PublicKey>
Ecdsa<CurveNid>::SecretKey::publicKey() const noexcept {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslCheck(i2d_PUBKEY_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslCheck(d2i_PUBKEY_bio(B.get(), &Res));

  return EvpPkeyPtr{Res};
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::KeyPair>
Ecdsa<CurveNid>::KeyPair::generate(OptionalRef<const Options>) noexcept {
  EvpPkeyCtxPtr ParamCtx{EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr)};
  EVP_PKEY_keygen_init(ParamCtx.get());
  EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ParamCtx.get(), CurveNid);

  EVP_PKEY *Key = nullptr;
  opensslCheck(EVP_PKEY_keygen(ParamCtx.get(), &Key));

  return EvpPkeyPtr{Key};
}

template <int CurveNid>
WasiCryptoExpect<EvpPkeyPtr>
Ecdsa<CurveNid>::KeyPair::checkValid(EvpPkeyPtr Ctx, bool) noexcept {
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  EC_KEY *EcCtx = EVP_PKEY_get0_EC_KEY(Ctx.get());
  ensureOrReturn(EcCtx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  // curve id check
  const EC_GROUP *Group = EC_KEY_get0_group(EcCtx);
  ensureOrReturn(Group, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(EC_GROUP_get_curve_name(Group) == CurveNid,
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  // have public key
  ensureOrReturn(EC_KEY_get0_public_key(EcCtx),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return {std::move(Ctx)};
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::KeyPair>
Ecdsa<CurveNid>::KeyPair::import(
    Span<const uint8_t> Encoded,
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

template <int CurveNid>
WasiCryptoExpect<SecretVec> Ecdsa<CurveNid>::KeyPair::exportData(
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

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::PublicKey>
Ecdsa<CurveNid>::KeyPair::publicKey() const noexcept {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslCheck(i2d_PUBKEY_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslCheck(d2i_PUBKEY_bio(B.get(), &Res));

  return EvpPkeyPtr{Res};
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::SecretKey>
Ecdsa<CurveNid>::KeyPair::secretKey() const noexcept {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslCheck(i2d_PrivateKey_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslCheck(d2i_PrivateKey_bio(B.get(), &Res));

  return EvpPkeyPtr{Res};
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::SignState>
Ecdsa<CurveNid>::KeyPair::openSignState() const noexcept {
  EvpMdCtxPtr SignCtx{EVP_MD_CTX_create()};
  opensslCheck(EVP_DigestSignInit(SignCtx.get(), nullptr, EVP_sha256(), nullptr,
                                  Ctx.get()));

  return SignCtx;
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::KeyPair>
Ecdsa<CurveNid>::KeyPair::importPkcs8(Span<const uint8_t> Encoded,
                                      bool Compressed) noexcept {
  return checkValid(EvpPkeyPtr{d2iPrivateKey(Encoded)}, Compressed);
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::KeyPair>
Ecdsa<CurveNid>::KeyPair::importPem(Span<const uint8_t> Encoded,
                                    bool Compressed) noexcept {
  return checkValid(EvpPkeyPtr{pemReadPrivateKey(Encoded)}, Compressed);
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::KeyPair>
Ecdsa<CurveNid>::KeyPair::importRaw(Span<const uint8_t> Encoded) noexcept {
  ensureOrReturn(Encoded.size() == SkSize, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  EcKeyPtr EcCtx{EC_KEY_new_by_curve_name(CurveNid)};
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

template <int CurveNid>
WasiCryptoExpect<SecretVec>
Ecdsa<CurveNid>::KeyPair::exportPkcs8() const noexcept {
  return i2dPrivateKey(Ctx.get());
}

template <int CurveNid>
WasiCryptoExpect<SecretVec>
Ecdsa<CurveNid>::KeyPair::exportPem() const noexcept {
  return pemWritePrivateKey(Ctx.get());
}

template <int CurveNid>
WasiCryptoExpect<SecretVec>
Ecdsa<CurveNid>::KeyPair::exportRaw() const noexcept {
  const BIGNUM *Sk = EC_KEY_get0_private_key(EVP_PKEY_get0_EC_KEY(Ctx.get()));
  SecretVec Res(SkSize);
  opensslCheck(BN_bn2bin(Sk, Res.data()));

  return Res;
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::Signature>
Ecdsa<CurveNid>::Signature::import(
    Span<const uint8_t> Encoded,
    __wasi_signature_encoding_e_t Encoding) noexcept {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW: {
    ensureOrReturn(Encoded.size() == RawSigSize,
                   __WASI_CRYPTO_ERRNO_INVALID_SIGNATURE);
    EcdsaSigPtr Sig{o2iEcdsaSig(Encoded)};
    ensureOrReturn(Sig, __WASI_CRYPTO_ERRNO_INVALID_SIGNATURE);
    return i2dEcdsaSig(Sig.get());
  }
  case __WASI_SIGNATURE_ENCODING_DER: {
    return std::vector<uint8_t>(Encoded.begin(), Encoded.end());
  }
  default:
    assumingUnreachable();
  }
}

template <int CurveNid>
WasiCryptoExpect<std::vector<uint8_t>> Ecdsa<CurveNid>::Signature::exportData(
    __wasi_signature_encoding_e_t Encoding) const noexcept {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW: {
    EcdsaSigPtr Sig{d2iEcdsaSig(Data)};
    ensureOrReturn(Sig, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
    return i2oEcdsaSig(Sig.get());
  }
  case __WASI_SIGNATURE_ENCODING_DER: {
    return Data;
  }
  default:
    assumingUnreachable();
  }
}

template <int CurveNid>
WasiCryptoExpect<void>
Ecdsa<CurveNid>::SignState::update(Span<const uint8_t> Data) noexcept {
  opensslCheck(EVP_DigestSignUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::Signature>
Ecdsa<CurveNid>::SignState::sign() noexcept {
  size_t Size;
  opensslCheck(EVP_DigestSignFinal(Ctx.get(), nullptr, &Size));

  auto Res = std::vector<uint8_t>(Size);
  opensslCheck(EVP_DigestSignFinal(Ctx.get(), Res.data(), &Size));

  return Res;
}

template <int CurveNid>
WasiCryptoExpect<void>
Ecdsa<CurveNid>::VerificationState::update(Span<const uint8_t> Data) noexcept {
  opensslCheck(EVP_DigestVerifyUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

template <int CurveNid>
WasiCryptoExpect<void>
Ecdsa<CurveNid>::VerificationState::verify(const Signature &Sig) noexcept {
  ensureOrReturn(
      EVP_DigestVerifyFinal(Ctx.get(), Sig.ref().data(), Sig.ref().size()),
      __WASI_CRYPTO_ERRNO_VERIFICATION_FAILED);
  return {};
}

template class Ecdsa<NID_X9_62_prime256v1>;
template class Ecdsa<NID_secp256k1>;
} // namespace Signatures
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
