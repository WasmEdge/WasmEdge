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
template <uint32_t CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::PublicKey>>
Ecdsa<CurveNid>::PublicKey::import(Span<const uint8_t> Encoded,
                                   __wasi_publickey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_PKCS8:
    return importPkcs8(Encoded);
  case __WASI_PUBLICKEY_ENCODING_PEM:
    return importPem(Encoded);
  case __WASI_PUBLICKEY_ENCODING_SEC:
  case __WASI_PUBLICKEY_ENCODING_COMPRESSED_SEC:
    return importRaw(Encoded);
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <uint32_t CurveNid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<CurveNid>::PublicKey::exportData(__wasi_publickey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_SEC:
    return exportSec();
  case __WASI_PUBLICKEY_ENCODING_COMPRESSED_SEC:
    return exportCompressedSec();
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

template <uint32_t CurveNid>
WasiCryptoExpect<std::unique_ptr<Signatures::VerificationState>>
Ecdsa<CurveNid>::PublicKey::openVerificationState() {
  EvpMdCtxPtr SignCtx{EVP_MD_CTX_create()};
  opensslAssuming(EVP_DigestVerifyInit(SignCtx.get(), nullptr, EVP_sha256(),
                                       nullptr, Ctx.get()));
  return std::make_unique<VerificationState>(std::move(SignCtx));
}

template <uint32_t CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::PublicKey>>
Ecdsa<CurveNid>::PublicKey::importPkcs8(Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};

  auto WriteRes = spanWriteToBio(Bio.get(), Encoded);
  if (!WriteRes) {
    return WasiCryptoUnexpect(WriteRes);
  }

  EvpPkeyPtr Ctx{d2i_PUBKEY_bio(Bio.get(), nullptr)};
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return std::make_unique<PublicKey>(std::move(Ctx));
}

template <uint32_t CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::PublicKey>>
Ecdsa<CurveNid>::PublicKey::importPem(Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};

  auto WriteRes = spanWriteToBio(Bio.get(), Encoded);
  if (!WriteRes) {
    return WasiCryptoUnexpect(WriteRes);
  }

  EvpPkeyPtr Ctx{PEM_read_bio_PUBKEY(Bio.get(), nullptr, nullptr, nullptr)};
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return std::make_unique<PublicKey>(std::move(Ctx));
}

template <uint32_t CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::PublicKey>>
Ecdsa<CurveNid>::PublicKey::importRaw(Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};

  auto WriteRes = spanWriteToBio(Bio.get(), Encoded);
  if (!WriteRes) {
    return WasiCryptoUnexpect(WriteRes);
  }

  EvpPkeyPtr Ctx{d2i_PUBKEY_bio(Bio.get(), nullptr)};
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return std::make_unique<PublicKey>(std::move(Ctx));
}

template <uint32_t CurveNid>
WasiCryptoExpect<std::vector<uint8_t>> Ecdsa<CurveNid>::PublicKey::exportSec() {
  EC_KEY_set_conv_form(EVP_PKEY_get0_EC_KEY(Ctx.get()),
                       POINT_CONVERSION_UNCOMPRESSED);
  std::vector<uint8_t> Res(static_cast<size_t>(i2d_PUBKEY(Ctx.get(), nullptr)));
  opensslAssuming(i2d_PUBKEY(Ctx.get(), addressOfTempory(Res.data())));
  return Res;
}

template <uint32_t CurveNid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<CurveNid>::PublicKey::exportCompressedSec() {
  EC_KEY_set_conv_form(EVP_PKEY_get0_EC_KEY(Ctx.get()),
                       POINT_CONVERSION_COMPRESSED);
  std::vector<uint8_t> Res(static_cast<size_t>(i2d_PUBKEY(Ctx.get(), nullptr)));
  opensslAssuming(i2d_PUBKEY(Ctx.get(), addressOfTempory(Res.data())));
  return Res;
}

template <uint32_t CurveNid>
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

template <uint32_t CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::SecretKey>>
Ecdsa<CurveNid>::SecretKey::importPkcs8(Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};

  auto WriteRes = spanWriteToBio(Bio.get(), Encoded);
  if (!WriteRes) {
    return WasiCryptoUnexpect(WriteRes);
  }

  EvpPkeyPtr Ctx{d2i_PrivateKey_bio(Bio.get(), nullptr)};
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return std::make_unique<SecretKey>(std::move(Ctx));
}

template <uint32_t CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::SecretKey>>
Ecdsa<CurveNid>::SecretKey::importPem(Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};

  auto WriteRes = spanWriteToBio(Bio.get(), Encoded);
  if (!WriteRes) {
    return WasiCryptoUnexpect(WriteRes);
  }

  EvpPkeyPtr Ctx{PEM_read_bio_PrivateKey(Bio.get(), nullptr, nullptr, nullptr)};
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return std::make_unique<SecretKey>(std::move(Ctx));
}

template <uint32_t CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::SecretKey>>
Ecdsa<CurveNid>::SecretKey::importRaw(Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};

  auto WriteRes = spanWriteToBio(Bio.get(), Encoded);
  if (!WriteRes) {
    return WasiCryptoUnexpect(WriteRes);
  }

  EvpPkeyPtr Ctx{d2i_PrivateKey_bio(Bio.get(), nullptr)};
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return std::make_unique<SecretKey>(std::move(Ctx));
}

template <uint32_t CurveNid>
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
template <uint32_t CurveNid>
WasiCryptoExpect<std::vector<uint8_t>>
Ecdsa<CurveNid>::SecretKey::exportPkcs8() {
  std::vector<uint8_t> Res(
      static_cast<size_t>(i2d_PrivateKey(Ctx.get(), nullptr)));
  opensslAssuming(i2d_PrivateKey(Ctx.get(), addressOfTempory(Res.data())));
  return Res;
}

template <uint32_t CurveNid>
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

template <uint32_t CurveNid>
WasiCryptoExpect<std::vector<uint8_t>> Ecdsa<CurveNid>::SecretKey::exportRaw() {
  std::vector<uint8_t> Res(
      static_cast<size_t>(i2d_PrivateKey(Ctx.get(), nullptr)));
  opensslAssuming(i2d_PrivateKey(Ctx.get(), addressOfTempory(Res.data())));
  return Res;
}

template <uint32_t CurveNid>
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

template <uint32_t CurveNid>
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

template <uint32_t CurveNid>
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

template <uint32_t CurveNid>
WasiCryptoExpect<std::unique_ptr<Signatures::PublicKey>>
Ecdsa<CurveNid>::KeyPair::publicKey() {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PUBKEY_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PUBKEY_bio(B.get(), &Res));

  return std::make_unique<PublicKey>(EvpPkeyPtr{Res});
}

template <uint32_t CurveNid>
WasiCryptoExpect<std::unique_ptr<SecretKey>>
Ecdsa<CurveNid>::KeyPair::secretKey() {
  BioPtr B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PrivateKey_bio(B.get(), Ctx.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PrivateKey_bio(B.get(), &Res));

  return std::make_unique<SecretKey>(EvpPkeyPtr{Res});
}

template <uint32_t CurveNid>
WasiCryptoExpect<std::unique_ptr<SignState>>
Ecdsa<CurveNid>::KeyPair::openSignState() {
  EvpMdCtxPtr SignCtx{EVP_MD_CTX_create()};
  opensslAssuming(EVP_DigestSignInit(SignCtx.get(), nullptr, EVP_sha256(),
                                     nullptr, Ctx.get()));

  return std::make_unique<SignState>(std::move(SignCtx));
}

template <uint32_t CurveNid>
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

template <uint32_t CurveNid>
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

template <uint32_t CurveNid>
WasiCryptoExpect<std::unique_ptr<typename Ecdsa<CurveNid>::KeyPair>>
Ecdsa<CurveNid>::KeyPair::importRaw(Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};

  auto WriteRes = spanWriteToBio(Bio.get(), Encoded);
  if (!WriteRes) {
    return WasiCryptoUnexpect(WriteRes);
  }

  EvpPkeyPtr Ctx{d2i_PrivateKey_bio(Bio.get(), nullptr)};
  ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return std::make_unique<KeyPair>(std::move(Ctx));
}

template <uint32_t CurveNid>
WasiCryptoExpect<std::vector<uint8_t>> Ecdsa<CurveNid>::KeyPair::exportPkcs8() {
  std::vector<uint8_t> Res(
      static_cast<size_t>(i2d_PrivateKey(Ctx.get(), nullptr)));
  opensslAssuming(i2d_PrivateKey(Ctx.get(), addressOfTempory(Res.data())));
  return Res;
}

template <uint32_t CurveNid>
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

template <uint32_t CurveNid>
WasiCryptoExpect<std::vector<uint8_t>> Ecdsa<CurveNid>::KeyPair::exportRaw() {
  std::vector<uint8_t> Res(
      static_cast<size_t>(i2d_PrivateKey(Ctx.get(), nullptr)));
  opensslAssuming(i2d_PrivateKey(Ctx.get(), addressOfTempory(Res.data())));
  return Res;
}

template <uint32_t CurveNid>
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

template <uint32_t CurveNid>
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

template <uint32_t CurveNid>
WasiCryptoExpect<void>
Ecdsa<CurveNid>::SignState::update(Span<const uint8_t> Data) {
  opensslAssuming(EVP_DigestSignUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

template <uint32_t CurveNid>
WasiCryptoExpect<std::unique_ptr<Signature>>
Ecdsa<CurveNid>::SignState::sign() {
  size_t Size;
  opensslAssuming(EVP_DigestSignFinal(Ctx.get(), nullptr, &Size));

  std::vector<uint8_t> Res(Size);
  opensslAssuming(EVP_DigestSignFinal(Ctx.get(), Res.data(), &Size));

  return std::make_unique<Signature>(std::move(Res));
}

template <uint32_t CurveNid>
WasiCryptoExpect<void>
Ecdsa<CurveNid>::VerificationState::update(Span<const uint8_t> Data) {
  opensslAssuming(EVP_DigestVerifyUpdate(Ctx.get(), Data.data(), Data.size()));
  return {};
}

template <uint32_t CurveNid>
WasiCryptoExpect<void> Ecdsa<CurveNid>::VerificationState::verify(
    std::shared_ptr<Signatures::Signature> Sig) {
  ensureOrReturn(Sig->alg() == getAlg(), __WASI_CRYPTO_ERRNO_INVALID_SIGNATURE);

  ensureOrReturn(
      EVP_DigestVerifyFinal(Ctx.get(), Sig->data().data(), Sig->data().size()),
      __WASI_CRYPTO_ERRNO_VERIFICATION_FAILED);
  return {};
}

template <uint32_t CurveNid> EvpPkeyPtr Ecdsa<CurveNid>::initEC() {
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
