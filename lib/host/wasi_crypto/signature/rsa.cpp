// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/rsa.h"

#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/evpwrapper.h"
#include "host/wasi_crypto/signature/keypair.h"
#include "host/wasi_crypto/signature/secretkey.h"
#include "host/wasi_crypto/util.h"
#include "openssl/x509.h"
#include "wasi_crypto/api.hpp"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {
namespace {
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
} // namespace

// raw secret scalar encoded as big endian, SEC-1, compressed SEC-1, unencrypted
// PKCS#8, PEM-encoded unencrypted PKCS#8
template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<
    std::unique_ptr<typename Rsa<PadMode, KeyBits, ShaNid>::PublicKey>>
Rsa<PadMode, KeyBits, ShaNid>::PublicKey::import(
    Span<const uint8_t> Encoded, __wasi_publickey_encoding_e_t Encoding) {
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

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<
    std::unique_ptr<typename Rsa<PadMode, KeyBits, ShaNid>::PublicKey>>
Rsa<PadMode, KeyBits, ShaNid>::PublicKey::importPkcs8(
    Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};

  auto WriteRes = spanWriteToBio(Bio.get(), Encoded);
  if (!WriteRes) {
    return WasiCryptoUnexpect(WriteRes);
  }

  EvpPkeyPtr Ctx{d2i_PUBKEY_bio(Bio.get(), nullptr)};
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(EVP_PKEY_id(Ctx.get()) == EVP_PKEY_RSA,
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return std::make_unique<PublicKey>(std::move(Ctx));
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<
    std::unique_ptr<typename Rsa<PadMode, KeyBits, ShaNid>::PublicKey>>
Rsa<PadMode, KeyBits, ShaNid>::PublicKey::importPem(
    Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};

  auto WriteRes = spanWriteToBio(Bio.get(), Encoded);
  if (!WriteRes) {
    return WasiCryptoUnexpect(WriteRes);
  }

  EvpPkeyPtr Ctx{PEM_read_bio_PUBKEY(Bio.get(), nullptr, nullptr, nullptr)};
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(EVP_PKEY_id(Ctx.get()) == EVP_PKEY_RSA,
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return std::make_unique<PublicKey>(std::move(Ctx));
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<
    std::unique_ptr<typename Rsa<PadMode, KeyBits, ShaNid>::PublicKey>>
Rsa<PadMode, KeyBits, ShaNid>::PublicKey::importLocal(Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::PublicKey::exportData(
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

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::PublicKey::exportPkcs8() {
  std::vector<uint8_t> Res(static_cast<size_t>(i2d_PUBKEY(Ctx.get(), nullptr)));
  opensslAssuming(i2d_PUBKEY(Ctx.get(), addressOfTempory(Res.data())));
  return Res;
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::PublicKey::exportPem() {
  BioPtr Bio{BIO_new(BIO_s_mem())};
  ensureOrReturn(PEM_write_bio_PUBKEY(Bio.get(), Ctx.get()),
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  std::vector<uint8_t> Pem(
      static_cast<size_t>(BIO_get_mem_data(Bio.get(), nullptr)));
  auto Res = bioWriteToSpan(Bio.get(), Pem);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return Pem;
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::PublicKey::exportLocal() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::unique_ptr<VerificationState>>
Rsa<PadMode, KeyBits, ShaNid>::PublicKey::openVerificationState() {
  EvpMdCtxPtr SignCtx{EVP_MD_CTX_create()};
  opensslAssuming(EVP_DigestVerifyInit(
      SignCtx.get(), nullptr, EVP_get_digestbynid(ShaNid), nullptr, Ctx.get()));

  return std::make_unique<VerificationState>(std::move(SignCtx));
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<
    std::unique_ptr<typename Rsa<PadMode, KeyBits, ShaNid>::SecretKey>>
Rsa<PadMode, KeyBits, ShaNid>::SecretKey::import(
    Span<const uint8_t> Encoded, __wasi_secretkey_encoding_e_t Encoding) {

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

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<
    std::unique_ptr<typename Rsa<PadMode, KeyBits, ShaNid>::SecretKey>>
Rsa<PadMode, KeyBits, ShaNid>::SecretKey::importPem(
    Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};

  auto WriteRes = spanWriteToBio(Bio.get(), Encoded);
  if (!WriteRes) {
    return WasiCryptoUnexpect(WriteRes);
  }

  EvpPkeyPtr Ctx{PEM_read_bio_PrivateKey(Bio.get(), nullptr, nullptr, nullptr)};
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(EVP_PKEY_id(Ctx.get()) == EVP_PKEY_RSA,
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return std::make_unique<SecretKey>(std::move(Ctx));
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<
    std::unique_ptr<typename Rsa<PadMode, KeyBits, ShaNid>::SecretKey>>
Rsa<PadMode, KeyBits, ShaNid>::SecretKey::importPkcs8(
    Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};

  auto WriteRes = spanWriteToBio(Bio.get(), Encoded);
  if (!WriteRes) {
    return WasiCryptoUnexpect(WriteRes);
  }

  EvpPkeyPtr Ctx{d2i_PrivateKey_bio(Bio.get(), nullptr)};
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(EVP_PKEY_id(Ctx.get()) == EVP_PKEY_RSA,
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return std::make_unique<SecretKey>(std::move(Ctx));
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<
    std::unique_ptr<typename Rsa<PadMode, KeyBits, ShaNid>::SecretKey>>
Rsa<PadMode, KeyBits, ShaNid>::SecretKey::importLocal(Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::SecretKey::exportData(
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

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::SecretKey::exportPem() {
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

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::SecretKey::exportPkcs8() {
  std::vector<uint8_t> Res(
      static_cast<size_t>(i2d_PrivateKey(Ctx.get(), nullptr)));
  opensslAssuming(i2d_PrivateKey(Ctx.get(), addressOfTempory(Res.data())));
  return Res;
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::SecretKey::exportLocal() {
  std::vector<uint8_t> Res(
      static_cast<size_t>(i2d_PrivateKey(Ctx.get(), nullptr)));
  opensslAssuming(i2d_PrivateKey(Ctx.get(), addressOfTempory(Res.data())));
  return Res;
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<
    std::unique_ptr<typename Rsa<PadMode, KeyBits, ShaNid>::KeyPair>>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::import(
    Span<const uint8_t> Encoded, __wasi_keypair_encoding_e_t Encoding) {

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

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<
    std::unique_ptr<typename Rsa<PadMode, KeyBits, ShaNid>::KeyPair>>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::importPem(Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};

  auto WriteRes = spanWriteToBio(Bio.get(), Encoded);
  if (!WriteRes) {
    return WasiCryptoUnexpect(WriteRes);
  }

  EvpPkeyPtr Ctx{PEM_read_bio_PrivateKey(Bio.get(), nullptr, nullptr, nullptr)};
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(EVP_PKEY_id(Ctx.get()) == EVP_PKEY_RSA,
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return std::make_unique<KeyPair>(std::move(Ctx));
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<
    std::unique_ptr<typename Rsa<PadMode, KeyBits, ShaNid>::KeyPair>>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::importPkcs8(
    Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};

  auto WriteRes = spanWriteToBio(Bio.get(), Encoded);
  if (!WriteRes) {
    return WasiCryptoUnexpect(WriteRes);
  }

  EvpPkeyPtr Ctx{d2i_PrivateKey_bio(Bio.get(), nullptr)};
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(EVP_PKEY_id(Ctx.get()) == EVP_PKEY_RSA,
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return std::make_unique<KeyPair>(std::move(Ctx));
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<
    std::unique_ptr<typename Rsa<PadMode, KeyBits, ShaNid>::KeyPair>>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::importLocal(Span<const uint8_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::unique_ptr<Signatures::SignState>>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::openSignState() {
  EvpMdCtxPtr SignCtx{EVP_MD_CTX_create()};

  opensslAssuming(EVP_DigestSignInit(
      SignCtx.get(), nullptr, EVP_get_digestbynid(ShaNid), nullptr, Ctx.get()));

  return std::make_unique<SignState>(std::move(SignCtx));
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<
    std::unique_ptr<typename Rsa<PadMode, KeyBits, ShaNid>::KeyPair>>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::generate(std::shared_ptr<Options>) {
  // notice: there cann't reuse initRsa(), careful
  EvpPkeyCtxPtr Ctx{EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr)};
  EVP_PKEY_keygen_init(Ctx.get());
  EVP_PKEY_CTX_set_rsa_padding(Ctx.get(), PadMode);
  EVP_PKEY_CTX_set_rsa_keygen_bits(Ctx.get(), KeyBits);
  EVP_PKEY_CTX_set_signature_md(Ctx.get(), getShaCtx());

  EVP_PKEY *Res = nullptr;
  EVP_PKEY_keygen(Ctx.get(), &Res);
  return std::make_unique<KeyPair>(EvpPkeyPtr{Res});
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::exportData(
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

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::exportPem() {
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

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::exportPkcs8() {
  std::vector<uint8_t> Res(
      static_cast<size_t>(i2d_PrivateKey(Ctx.get(), nullptr)));
  opensslAssuming(i2d_PrivateKey(Ctx.get(), addressOfTempory(Res.data())));
  return Res;
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::exportLocal() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::unique_ptr<Signatures::PublicKey>>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::publicKey() {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PUBKEY_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PUBKEY_bio(B.get(), &Res));
  ensureOrReturn(Res, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return std::make_unique<PublicKey>(EvpPkeyPtr{Res});
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::unique_ptr<Signatures::SecretKey>>
Rsa<PadMode, KeyBits, ShaNid>::KeyPair::secretKey() {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PrivateKey_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PrivateKey_bio(B.get(), &Res));

  return std::make_unique<SecretKey>(EvpPkeyPtr{Res});
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<
    std::unique_ptr<typename Rsa<PadMode, KeyBits, ShaNid>::Signature>>
Rsa<PadMode, KeyBits, ShaNid>::Signature::import(
    Span<const uint8_t> Encoded, __wasi_signature_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW:
    ensureOrReturn(Encoded.size() == getKeySize(),
                   __WASI_CRYPTO_ERRNO_INVALID_SIGNATURE);
    return std::make_unique<Signature>(
        std::vector<uint8_t>{Encoded.begin(), Encoded.end()});
  case __WASI_SIGNATURE_ENCODING_DER:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  default:
    assumingUnreachable();
  }
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::vector<uint8_t>>
Rsa<PadMode, KeyBits, ShaNid>::Signature::exportData(
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

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<void>
Rsa<PadMode, KeyBits, ShaNid>::SignState::update(Span<const uint8_t> Data) {
  opensslAssuming(EVP_DigestSignUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<std::unique_ptr<Signatures::Signature>>
Rsa<PadMode, KeyBits, ShaNid>::SignState::sign() {
  size_t Siz;
  opensslAssuming(EVP_DigestSignFinal(Ctx.get(), nullptr, &Siz));

  std::vector<uint8_t> Res(Siz);

  opensslAssuming(EVP_DigestSignFinal(Ctx.get(), Res.data(), &Siz));

  return std::make_unique<Signature>(std::move(Res));
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<void> Rsa<PadMode, KeyBits, ShaNid>::VerificationState::update(
    Span<const uint8_t> Data) {
  opensslAssuming(EVP_DigestVerifyUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

template <int PadMode, int KeyBits, int ShaNid>
WasiCryptoExpect<void> Rsa<PadMode, KeyBits, ShaNid>::VerificationState::verify(
    std::shared_ptr<Signatures::Signature> Sig) {
  ensureOrReturn(Sig->alg() == getAlg(), __WASI_CRYPTO_ERRNO_INVALID_SIGNATURE);
  ensureOrReturn(
      EVP_DigestVerifyFinal(Ctx.get(), Sig->data().data(), Sig->data().size()),
      __WASI_CRYPTO_ERRNO_VERIFICATION_FAILED);

  return {};
}

template <int PadMode, int KeyBits, int ShaNid>
EvpPkeyPtr Rsa<PadMode, KeyBits, ShaNid>::initRsa() {
  EvpPkeyCtxPtr PCtx{EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr)};
  EVP_PKEY_CTX_set_rsa_padding(PCtx.get(), PadMode);
  EVP_PKEY_CTX_set_rsa_keygen_bits(PCtx.get(), KeyBits);
  EVP_PKEY_CTX_set_signature_md(PCtx.get(), getShaCtx());

  EVP_PKEY *Res = nullptr;
  EVP_PKEY_paramgen(PCtx.get(), &Res);
  return EvpPkeyPtr{Res};
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
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
