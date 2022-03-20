// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/signatures/rsa.h"
#include "host/wasi_crypto/utils/error.h"
#include "host/wasi_crypto/utils/evp_wrapper.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<typename Rsa<PadMode, KeyBits, ShaNid>::PublicKey>
Rsa<PadMode, KeyBits, ShaNid>::PublicKey::import(
    Span<const uint8_t> Encoded,
    __wasi_publickey_encoding_e_t Encoding) noexcept {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_PKCS8:
    return importPkcs8(Encoded);
  case __WASI_PUBLICKEY_ENCODING_PEM:
    return importPem(Encoded);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<typename Rsa<PadMode, KeyBits, ShaNid>::PublicKey>
Rsa<PadMode, KeyBits, ShaNid>::PublicKey::importPkcs8(
    Span<const uint8_t> Encoded) noexcept {
  return checkValid(EvpPkeyPtr{d2iPUBKEY(Encoded)});
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<typename Rsa<PadMode, KeyBits, ShaNid>::PublicKey>
Rsa<PadMode, KeyBits, ShaNid>::PublicKey::importPem(
    Span<const uint8_t> Encoded) noexcept {
  return checkValid(EvpPkeyPtr{pemReadPUBKEY(Encoded)});
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<EvpPkeyPtr>
Rsa<PadMode, KeyBits, ShaNid>::PublicKey::checkValid(EvpPkeyPtr Ctx) noexcept {
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  RSA *RsaKey = EVP_PKEY_get0_RSA(Ctx.get());
  ensureOrReturn(RsaKey, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(RSA_bits(RsaKey) == KeyBits, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  EvpPkeyCtxPtr PkeyCtx{EVP_PKEY_CTX_new(Ctx.get(), nullptr)};
  int InnerPadMode;
  ensureOrReturn(EVP_PKEY_CTX_get_rsa_padding(
                     PkeyCtx.get(), static_cast<void *>(&InnerPadMode)),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(InnerPadMode == PadMode, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return {std::move(Ctx)};
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<void>
Rsa<PadMode, KeyBits, ShaNid>::PublicKey::verify() const noexcept {
  ensureOrReturn(RSA_check_key(EVP_PKEY_get0_RSA(Ctx.get())),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return {};
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::PublicKey::exportData(
    __wasi_publickey_encoding_e_t Encoding) const noexcept {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_PKCS8:
    return exportPkcs8();
  case __WASI_PUBLICKEY_ENCODING_PEM:
    return exportPem();
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::PublicKey::exportPkcs8() const noexcept {
  return i2dPUBKEY(Ctx.get());
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::PublicKey::exportPem() const noexcept {
  return pemWritePUBKEY(Ctx.get());
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<typename Rsa<PadMode, KeyBits, ShaNid>::VerificationState>
Rsa<PadMode, KeyBits, ShaNid>::PublicKey::openVerificationState() noexcept {
  EvpMdCtxPtr SignCtx{EVP_MD_CTX_create()};
  opensslAssuming(EVP_DigestVerifyInit(
      SignCtx.get(), nullptr, EVP_get_digestbynid(ShaNid), nullptr, Ctx.get()));

  return SignCtx;
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<typename Rsa<PadMode, KeyBits, ShaNid>::SecretKey>
Rsa<PadMode, KeyBits, ShaNid>::SecretKey::import(
    Span<const uint8_t> Encoded,
    __wasi_secretkey_encoding_e_t Encoding) noexcept {
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_PKCS8:
    return importPkcs8(Encoded);
  case __WASI_SECRETKEY_ENCODING_PEM:
    return importPem(Encoded);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<typename Rsa<PadMode, KeyBits, ShaNid>::SecretKey>
Rsa<PadMode, KeyBits, ShaNid>::SecretKey::importPem(
    Span<const uint8_t> Encoded) noexcept {
  return checkValid(EvpPkeyPtr{pemReadPrivateKey(Encoded)});
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<typename Rsa<PadMode, KeyBits, ShaNid>::SecretKey>
Rsa<PadMode, KeyBits, ShaNid>::SecretKey::importPkcs8(
    Span<const uint8_t> Encoded) noexcept {
  return checkValid(EvpPkeyPtr{d2iPrivateKey(Encoded)});
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<EvpPkeyPtr>
Rsa<PadMode, KeyBits, ShaNid>::SecretKey::checkValid(EvpPkeyPtr Ctx) noexcept {
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  RSA *RsaKey = EVP_PKEY_get0_RSA(Ctx.get());
  ensureOrReturn(RsaKey, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(RSA_bits(RsaKey) == KeyBits, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  EvpPkeyCtxPtr PkeyCtx{EVP_PKEY_CTX_new(Ctx.get(), nullptr)};
  int InnerPadMode;
  ensureOrReturn(EVP_PKEY_CTX_get_rsa_padding(
                     PkeyCtx.get(), static_cast<void *>(&InnerPadMode)),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(InnerPadMode == PadMode, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return {std::move(Ctx)};
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<typename Rsa<PadMode, KeyBits, ShaNid>::KeyPair>
Rsa<PadMode, KeyBits, ShaNid>::SecretKey::toKeyPair(PublicKey &) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::SecretKey::exportData(
    __wasi_secretkey_encoding_e_t Encoding) const noexcept {
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_PKCS8:
    return exportPkcs8();
  case __WASI_SECRETKEY_ENCODING_PEM:
    return exportPem();
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<typename Rsa<PadMode, KeyBits, ShaNid>::PublicKey>
Rsa<PadMode, KeyBits, ShaNid>::SecretKey::publicKey() const noexcept {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PUBKEY_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PUBKEY_bio(B.get(), &Res));

  return EvpPkeyPtr{Res};
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::SecretKey::exportPem() const noexcept {
  return pemWritePrivateKey(Ctx.get());
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::SecretKey::exportPkcs8() const noexcept {
  return i2dPrivateKey(Ctx.get());
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<typename Rsa<PadMode, KeyBits, ShaNid>::KeyPair>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::import(
    Span<const uint8_t> Encoded,
    __wasi_keypair_encoding_e_t Encoding) noexcept {
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_PKCS8:
    return importPkcs8(Encoded);
  case __WASI_KEYPAIR_ENCODING_PEM:
    return importPem(Encoded);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<typename Rsa<PadMode, KeyBits, ShaNid>::KeyPair>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::importPem(
    Span<const uint8_t> Encoded) noexcept {
  return checkValid(EvpPkeyPtr{d2iPrivateKey(Encoded)});
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<typename Rsa<PadMode, KeyBits, ShaNid>::KeyPair>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::importPkcs8(
    Span<const uint8_t> Encoded) noexcept {
  return checkValid(EvpPkeyPtr{d2iPrivateKey(Encoded)});
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<EvpPkeyPtr>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::checkValid(EvpPkeyPtr Ctx) noexcept {
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  RSA *RsaKey = EVP_PKEY_get0_RSA(Ctx.get());
  ensureOrReturn(RsaKey, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(RSA_bits(RsaKey) == KeyBits, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  EvpPkeyCtxPtr PkeyCtx{EVP_PKEY_CTX_new(Ctx.get(), nullptr)};
  int InnerPadMode;
  ensureOrReturn(EVP_PKEY_CTX_get_rsa_padding(
                     PkeyCtx.get(), static_cast<void *>(&InnerPadMode)),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(InnerPadMode == PadMode, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  return {std::move(Ctx)};
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<typename Rsa<PadMode, KeyBits, ShaNid>::SignState>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::openSignState() noexcept {
  EvpMdCtxPtr SignCtx{EVP_MD_CTX_create()};
  opensslAssuming(EVP_DigestSignInit(
      SignCtx.get(), nullptr, EVP_get_digestbynid(ShaNid), nullptr, Ctx.get()));

  return SignCtx;
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<typename Rsa<PadMode, KeyBits, ShaNid>::KeyPair>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::generate(
    OptionalRef<Options>) noexcept {
  EvpPkeyCtxPtr Ctx{EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr)};
  EVP_PKEY_keygen_init(Ctx.get());
  EVP_PKEY_CTX_set_rsa_padding(Ctx.get(), PadMode);
  EVP_PKEY_CTX_set_rsa_keygen_bits(Ctx.get(), KeyBits);
  EVP_PKEY_CTX_set_signature_md(Ctx.get(), getShaCtx());

  EVP_PKEY *Res = nullptr;
  EVP_PKEY_keygen(Ctx.get(), &Res);
  return EvpPkeyPtr{Res};
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::exportData(
    __wasi_keypair_encoding_e_t Encoding) const noexcept {
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_PKCS8:
    return exportPkcs8();
  case __WASI_KEYPAIR_ENCODING_PEM:
    return exportPem();
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::exportPem() const noexcept {
  return pemWritePrivateKey(Ctx.get());
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::exportPkcs8() const noexcept {
  return i2dPrivateKey(Ctx.get());
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<typename Rsa<PadMode, KeyBits, ShaNid>::PublicKey>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::publicKey() const noexcept {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PUBKEY_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PUBKEY_bio(B.get(), &Res));
  ensureOrReturn(Res, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return EvpPkeyPtr{Res};
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<typename Rsa<PadMode, KeyBits, ShaNid>::SecretKey>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::secretKey() const noexcept {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PrivateKey_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PrivateKey_bio(B.get(), &Res));

  return EvpPkeyPtr{Res};
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<typename Rsa<PadMode, KeyBits, ShaNid>::Signature>
Rsa<PadMode, KeyBits, ShaNid>::Signature::import(
    Span<const uint8_t> Encoded,
    __wasi_signature_encoding_e_t Encoding) noexcept {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW:
    ensureOrReturn(Encoded.size() == getKeySize(),
                   __WASI_CRYPTO_ERRNO_INVALID_SIGNATURE);
    return std::vector<uint8_t>{Encoded.begin(), Encoded.end()};
  case __WASI_SIGNATURE_ENCODING_DER:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  default:
    assumingUnreachable();
  }
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::Signature::exportData(
    __wasi_signature_encoding_e_t Encoding) const noexcept {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW:
    return Data;
  case __WASI_SIGNATURE_ENCODING_DER:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  default:
    assumingUnreachable();
  }
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<void> Rsa<PadMode, KeyBits, ShaNid>::SignState::update(
    Span<const uint8_t> Data) noexcept {
  opensslAssuming(EVP_DigestSignUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<typename Rsa<PadMode, KeyBits, ShaNid>::Signature>
Rsa<PadMode, KeyBits, ShaNid>::SignState::sign() noexcept {
  size_t Siz;
  opensslAssuming(EVP_DigestSignFinal(Ctx.get(), nullptr, &Siz));

  std::vector<uint8_t> Res(Siz);

  opensslAssuming(EVP_DigestSignFinal(Ctx.get(), Res.data(), &Siz));

  return Res;
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<void> Rsa<PadMode, KeyBits, ShaNid>::VerificationState::update(
    Span<const uint8_t> Data) noexcept {
  opensslAssuming(EVP_DigestVerifyUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<void> Rsa<PadMode, KeyBits, ShaNid>::VerificationState::verify(
    Signature &Sig) noexcept {
  ensureOrReturn(
      EVP_DigestVerifyFinal(Ctx.get(), Sig.ref().data(), Sig.ref().size()),
      __WASI_CRYPTO_ERRNO_VERIFICATION_FAILED);

  return {};
}

template class Rsa<RSA_PKCS1_PADDING, 2048, NID_sha256>;
template class Rsa<RSA_PKCS1_PADDING, 2048, NID_sha384>;
template class Rsa<RSA_PKCS1_PADDING, 2048, NID_sha512>;

template class Rsa<RSA_PKCS1_PADDING, 3072, NID_sha384>;
template class Rsa<RSA_PKCS1_PADDING, 3072, NID_sha512>;

template class Rsa<RSA_PKCS1_PADDING, 4096, NID_sha512>;

template class Rsa<RSA_PKCS1_PSS_PADDING, 2048, NID_sha256>;
template class Rsa<RSA_PKCS1_PSS_PADDING, 2048, NID_sha384>;
template class Rsa<RSA_PKCS1_PSS_PADDING, 2048, NID_sha512>;

template class Rsa<RSA_PKCS1_PSS_PADDING, 3072, NID_sha384>;
template class Rsa<RSA_PKCS1_PSS_PADDING, 3072, NID_sha512>;

template class Rsa<RSA_PKCS1_PSS_PADDING, 4096, NID_sha512>;

} // namespace Signatures
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
