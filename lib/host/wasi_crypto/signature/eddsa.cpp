// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/eddsa.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<std::unique_ptr<EddsaSignaturePublicKey>>
WASICrypto::EddsaSignaturePublicKey::import(
    SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
    __wasi_publickey_encoding_e_t Encoding) {
  auto Pk = EddsaPkCtx::import(Alg, Encoded, Encoding);
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  return std::make_unique<EddsaSignaturePublicKey>(std::move(*Pk));
}

WasiCryptoExpect<std::vector<uint8_t>>
EddsaSignaturePublicKey::exportData(__wasi_publickey_encoding_e_t Encoding) {
  return Ctx.exportData(Encoding);
}

WasiCryptoExpect<SignatureVerificationState>
EddsaSignaturePublicKey::asState() {
  auto Res = Ctx.asVerification();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureVerificationState{
      std::make_unique<EddsaSignatureVerificationState>(std::move(*Res))};
}

WasiCryptoExpect<SignatureSecretKey>
EddsaSignatureSecretKey::import(SignatureAlgorithm Alg,
                                Span<const uint8_t> Encoded,
                                __wasi_secretkey_encoding_e_t Encoding) {
  auto Sk = EddsaSkCtx::import(Alg, Encoded, Encoding);
  if (!Sk) {
    return WasiCryptoUnexpect(Sk);
  }

  return SignatureSecretKey{
      std::make_unique<EddsaSignatureSecretKey>(std::move(*Sk))};
}

WasiCryptoExpect<std::vector<uint8_t>>
EddsaSignatureSecretKey::exportData(__wasi_secretkey_encoding_e_t Encoding) {
  return Ctx.exportData(Encoding);
}

WasiCryptoExpect<SignatureKeyPair>
EddsaSignatureKeyPair::generate(SignatureAlgorithm Alg,
                                std::optional<SignatureOptions>) {
  auto Res = EddsaKpCtx::generate(Alg);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureKeyPair{
      std::make_unique<EddsaSignatureKeyPair>(std::move(*Res))};
}

WasiCryptoExpect<SignatureKeyPair>
EddsaSignatureKeyPair::import(SignatureAlgorithm Alg,
                              Span<const uint8_t> Encoded,
                              __wasi_keypair_encoding_e_t Encoding) {
  auto Res = EddsaKpCtx::import(Alg, Encoded, Encoding);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureKeyPair{
      std::make_unique<EddsaSignatureKeyPair>(std::move(*Res))};
}

WasiCryptoExpect<std::vector<uint8_t>>
EddsaSignatureKeyPair::exportData(__wasi_keypair_encoding_e_t Encoding) {
  return Ctx.exportData(Encoding);
}

WasiCryptoExpect<SignaturePublicKey> EddsaSignatureKeyPair::publicKey() {
  auto Res = Ctx.publicKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignaturePublicKey{
      std::make_unique<EddsaSignaturePublicKey>(std::move(*Res))};
}

WasiCryptoExpect<SignatureSecretKey> EddsaSignatureKeyPair::secretKey() {
  auto Res = Ctx.secretKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureSecretKey{
      std::make_unique<EddsaSignatureSecretKey>(std::move(*Res))};
}

WasiCryptoExpect<SignatureState> EddsaSignatureKeyPair::asState() {
  auto Res = Ctx.asSignState();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SignatureState{std::make_unique<EddsaSignatureState>(std::move(*Res))};
}

WasiCryptoExpect<Signature>
EddsaSignature::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                       __wasi_signature_encoding_e_t Encoding) {
  auto Res = EddsaSignCtx::import(Alg, Encoded, Encoding);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return Signature{std::make_unique<EddsaSignature>(std::move(*Res))};
}

WasiCryptoExpect<std::vector<uint8_t>>
EddsaSignature::exportData(__wasi_signature_encoding_e_t Encoding) {
  return Ctx.exportData(Encoding);
}

WasiCryptoExpect<void> EddsaSignatureState::update(Span<const uint8_t> Input) {
  return Ctx.update(Input);
}

WasiCryptoExpect<Signature> EddsaSignatureState::sign() {
  auto Res = Ctx.sign();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }
  return Signature{std::make_unique<EddsaSignature>(std::move(*Res))};
}

WasiCryptoExpect<void>
EddsaSignatureVerificationState::update(Span<const uint8_t> Input) {
  return Ctx.update(Input);
}

WasiCryptoExpect<void>
EddsaSignatureVerificationState::verify(std::unique_ptr<Signature::Base> &Sig) {
  return Ctx.verify(Sig->asRef());
}

namespace {

EVP_PKEY *initED(SignatureAlgorithm) {
  //  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> PCtx{
  //      EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr)};
  //
  //  opensslAssuming(PCtx);
  //  opensslAssuming(EVP_PKEY_paramgen_init(PCtx.get()));
  //
  //  EVP_PKEY *Params = nullptr;
  //  opensslAssuming(EVP_PKEY_paramgen(PCtx.get(), &Params));
  //  return Params;
  return nullptr;
}

} // namespace

// raw secret scalar encoded as big endian, SEC-1, compressed SEC-1, unencrypted
// PKCS#8, PEM-encoded unencrypted PKCS#8
WasiCryptoExpect<EddsaPkCtx>
EddsaPkCtx::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                   __wasi_publickey_encoding_e_t Encoding) {
  EVP_PKEY *Pk = initED(Alg);
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    const uint8_t *Temp = Encoded.data();
    Pk = d2i_PublicKey(EVP_PKEY_ED25519, &Pk, &Temp, Encoded.size());
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

  return EddsaPkCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{Pk}};
}

WasiCryptoExpect<std::vector<uint8_t>>
EddsaPkCtx::exportData(__wasi_publickey_encoding_e_t Encoding) {
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

WasiCryptoExpect<EddsaVerificationCtx> EddsaPkCtx::asVerification() {
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> SignCtx{EVP_MD_CTX_create()};
  opensslAssuming(SignCtx);

  opensslAssuming(
      EVP_DigestVerifyInit(SignCtx.get(), nullptr, nullptr, nullptr, Pk.get()));
  return EddsaVerificationCtx{std::move(SignCtx)};
}

WasiCryptoExpect<EddsaSkCtx>
EddsaSkCtx::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                   __wasi_secretkey_encoding_e_t Encoding) {
  EVP_PKEY *Sk = initED(Alg);
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW: {
    const uint8_t *Temp = Encoded.data();
    Sk = d2i_PrivateKey(EVP_PKEY_ED25519, &Sk, &Temp, Encoded.size());
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

  return EddsaSkCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{Sk}};
}

WasiCryptoExpect<std::vector<uint8_t>>
EddsaSkCtx::exportData(__wasi_secretkey_encoding_e_t Encoding) {
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

WasiCryptoExpect<EddsaKpCtx>
EddsaKpCtx::import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
                   __wasi_keypair_encoding_e_t Encoding) {
  EVP_PKEY *Kp = initED(Alg);
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    const uint8_t *Temp = Encoded.data();
    Kp = d2i_PrivateKey(EVP_PKEY_ED25519, &Kp, &Temp, Encoded.size());
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

  return EddsaKpCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{Kp}};
}

WasiCryptoExpect<EddsaSignStateCtx> EddsaKpCtx::asSignState() {
  OpenSSLUniquePtr<EVP_MD_CTX, EVP_MD_CTX_free> SignCtx{EVP_MD_CTX_create()};
  opensslAssuming(SignCtx);

  opensslAssuming(
      EVP_DigestSignInit(SignCtx.get(), nullptr, nullptr, nullptr, Kp.get()));

  return EddsaSignStateCtx{std::move(SignCtx)};
}

WasiCryptoExpect<EddsaKpCtx> EddsaKpCtx::generate(SignatureAlgorithm) {

  // Generate Key
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> KCtx{
      EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr)};
  opensslAssuming(KCtx);
  opensslAssuming(EVP_PKEY_keygen_init(KCtx.get()));

  EVP_PKEY *Key = nullptr;
  opensslAssuming(EVP_PKEY_keygen(KCtx.get(), &Key));

  return EddsaKpCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{Key}};
}

WasiCryptoExpect<std::vector<uint8_t>>
EddsaKpCtx::exportData(__wasi_keypair_encoding_e_t Encoding) {
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

WasiCryptoExpect<EddsaPkCtx> EddsaKpCtx::publicKey() {
  OpenSSLUniquePtr<BIO, BIO_free> B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PUBKEY_bio(B.get(), Kp.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PUBKEY_bio(B.get(), &Res));

  return EddsaPkCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>(Res)};
}

WasiCryptoExpect<EddsaSkCtx> EddsaKpCtx::secretKey() {
  OpenSSLUniquePtr<BIO, BIO_free> B{BIO_new(BIO_s_mem())};
  opensslAssuming(i2d_PrivateKey_bio(B.get(), Kp.get()));

  EVP_PKEY *Res = nullptr;
  opensslAssuming(d2i_PrivateKey_bio(B.get(), &Res));

  return EddsaSkCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{Res}};
}

WasiCryptoExpect<EddsaSignCtx>
EddsaSignCtx::import(SignatureAlgorithm, Span<const uint8_t>,
                     __wasi_signature_encoding_e_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::vector<uint8_t>>
EddsaSignCtx::exportData(__wasi_signature_encoding_e_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<void> EddsaSignStateCtx::update(Span<const uint8_t> Data) {
  // Notice: Ecdsa is oneshot in OpenSSL, we need a cache for update instead of
  // call `EVP_DigestSignUpdate`
  Cache.insert(Cache.end(), Data.begin(), Data.end());
  return {};
}

WasiCryptoExpect<EddsaSignCtx> EddsaSignStateCtx::sign() {
  size_t Size;
  opensslAssuming(
      EVP_DigestSign(MdCtx.get(), nullptr, &Size, Cache.data(), Cache.size()));

  std::vector<uint8_t> Res(Size);
  opensslAssuming(EVP_DigestSign(MdCtx.get(), Res.data(), &Size, Cache.data(),
                                 Cache.size()));

  return Res;
}

WasiCryptoExpect<void> EddsaVerificationCtx::update(Span<const uint8_t> Data) {
  Cache.insert(Cache.end(), Data.begin(), Data.end());
  return {};
}

WasiCryptoExpect<void> EddsaVerificationCtx::verify(Span<const uint8_t> Data) {
  opensslAssuming(EVP_DigestVerify(MdCtx.get(), Data.data(), Data.size(),
                                   Cache.data(), Cache.size()));

  return {};
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
