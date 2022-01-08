// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/dh/x25519.h"
#include "host/wasi_crypto/wrapper/x25519.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<KxPublicKey>
X25519PublicKey::Builder::import(Span<uint8_t const> Raw,
                                 __wasi_publickey_encoding_e_t Encoding) {
  if (Raw.size() != X25519PKCtx::Len) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }

  auto Pk = X25519PublicKey::import(Alg, Raw, Encoding);
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  return KxPublicKey{std::make_unique<X25519PublicKey>(std::move(*Pk))};
}

WasiCryptoExpect<X25519PublicKey>
X25519PublicKey::import(KxAlgorithm Alg, Span<const uint8_t> Raw,
                        __wasi_publickey_encoding_e_t Encoding) {

  auto Res = X25519PKCtx::import(Raw, Encoding);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return X25519PublicKey{Alg, std::move(*Res)};
}

WasiCryptoExpect<std::vector<uint8_t>> X25519PublicKey::exportData() {
  return Ctx.exportData();
}

WasiCryptoExpect<void> X25519PublicKey::verify() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<KxSecretKey>
X25519SecretKey::Builder::import(Span<const uint8_t> Raw,
                                 __wasi_secretkey_encoding_e_t Encoding) {
  if (Raw.size() != X25519SKCtx::Len) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }

  auto Res = X25519SKCtx::import(Raw, Encoding);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return KxSecretKey{std::make_unique<X25519SecretKey>(Alg, std::move(*Res))};
}

WasiCryptoExpect<X25519SecretKey>
X25519SecretKey::import(KxAlgorithm Alg, Span<const uint8_t> Raw,
                        __wasi_secretkey_encoding_e_t Encoding) {
  auto Res = X25519SKCtx::import(Raw, Encoding);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return X25519SecretKey{Alg, std::move(*Res)};
}

WasiCryptoExpect<KxPublicKey> X25519SecretKey::publicKey() {
  auto Res = producePublicKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return KxPublicKey{std::make_unique<X25519PublicKey>(std::move(*Res))};
}

WasiCryptoExpect<__wasi_size_t> X25519SecretKey::len() {
  return X25519SKCtx::Len;
}

WasiCryptoExpect<Span<const uint8_t>> X25519SecretKey::exportData() {
  return Ctx.exportData();
}

WasiCryptoExpect<std::vector<uint8_t>>
X25519SecretKey::dh(std::unique_ptr<KxPublicKey::Base> &KxPk) {
  auto *Res = dynamic_cast<X25519PublicKey *>(KxPk.get());
  if (Res == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }

  // TODO: may not friend?
  return Ctx.dh(Res->Ctx);
}

WasiCryptoExpect<X25519PublicKey> X25519SecretKey::producePublicKey() {
  auto Res = Ctx.producePublicKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return X25519PublicKey{Alg, std::move(*Res)};
}

WasiCryptoExpect<KxKeyPair>
X25519KeyPair::Builder::generate(std::optional<KxOptions> /*TODO:Options*/) {
  auto KpCtx = X25519KpCtx::generate(Alg);
  if (!KpCtx) {
    return WasiCryptoUnexpect(KpCtx);
  }

  //  auto Kp = X25519KeyPair{Alg, *Pk, *Sk};
  return KxKeyPair{std::make_unique<X25519KeyPair>(Alg, std::move(*KpCtx))};
}

WasiCryptoExpect<KxKeyPair> X25519KeyPair::import(KxAlgorithm,
                                                  Span<const uint8_t>,
                                                  __wasi_keypair_encoding_e_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<void> X25519KeyPair::verify() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<KxPublicKey> X25519KeyPair::publicKey() {
  auto Res = Kp.publicKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return KxPublicKey{std::make_unique<X25519PublicKey>(Alg, std::move(*Res))};
}

WasiCryptoExpect<KxSecretKey> X25519KeyPair::secretKey() {
  auto Res = Kp.secretKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return KxSecretKey{std::make_unique<X25519SecretKey>(Alg, std::move(*Res))};
}


WasiCryptoExpect<X25519PKCtx>
X25519PKCtx::import(Span<const uint8_t> Raw,
                    __wasi_publickey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW:
    break;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  }

  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Pk{EVP_PKEY_new_raw_public_key(
      EVP_PKEY_X25519, nullptr, Raw.data(), Raw.size())};
  assuming(Pk);

  return X25519PKCtx{std::move(Pk)};
}

WasiCryptoExpect<std::vector<uint8_t>> X25519PKCtx::exportData() {
  size_t Size;
  assuming(EVP_PKEY_get_raw_public_key(Pk.get(), nullptr, &Size));

  std::vector<uint8_t> Res;
  Res.reserve(Size);
  Res.resize(Size);
  assuming(EVP_PKEY_get_raw_public_key(Pk.get(), Res.data(), &Size));
  return Res;
}

WasiCryptoExpect<X25519SKCtx>
X25519SKCtx::import(Span<const uint8_t> Raw,
                    __wasi_secretkey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW:
    break;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  }

  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Sk{EVP_PKEY_new_raw_private_key(
      EVP_PKEY_X25519, nullptr, Raw.data(), Raw.size())};
  assuming(Sk);
  return X25519SKCtx{std::move(Sk)};
}

WasiCryptoExpect<std::vector<uint8_t>> X25519SKCtx::exportData() {
  size_t Size;
  assuming(EVP_PKEY_get_raw_private_key(Sk.get(), nullptr, &Size));

  std::vector<uint8_t> Res;
  Res.reserve(Size);
  Res.resize(Size);
  assuming(EVP_PKEY_get_raw_private_key(Sk.get(), Res.data(), &Size));
  return Res;
}

WasiCryptoExpect<X25519PKCtx> X25519SKCtx::producePublicKey() {
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> PCtx{
      EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr)};
  assuming(PCtx);
  assuming(EVP_PKEY_keygen_init(PCtx.get()));

  EVP_PKEY *P = nullptr;
  assuming(EVP_PKEY_keygen(PCtx.get(), &P));

  return X25519PKCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{P}};
}

WasiCryptoExpect<std::vector<uint8_t>> X25519SKCtx::dh(X25519PKCtx &Pk) {
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx{
      EVP_PKEY_CTX_new(Sk.get(), nullptr)};
  assuming(Ctx);
  assuming(EVP_PKEY_derive_init(Ctx.get()));

  // normally peer will be a public key
  assuming(EVP_PKEY_derive_set_peer(Ctx.get(), Pk.Pk.get()));

  size_t Size;
  assuming(EVP_PKEY_derive(Ctx.get(), nullptr, &Size));

  std::vector<uint8_t> Res;
  Res.reserve(Size);
  Res.resize(Size);
  assuming(EVP_PKEY_derive(Ctx.get(), Res.data(), &Size));

  return Res;
}

WasiCryptoExpect<X25519PKCtx> X25519KpCtx::publicKey() {
  size_t Size;
  assuming(EVP_PKEY_get_raw_public_key(Ctx.get(), nullptr, &Size));

  std::vector<uint8_t> Res;
  Res.reserve(Size);
  Res.resize(Size);
  assuming(EVP_PKEY_get_raw_public_key(Ctx.get(), Res.data(), &Size));

  return X25519PKCtx::import(Res, __WASI_PUBLICKEY_ENCODING_RAW);
}

WasiCryptoExpect<X25519SKCtx> X25519KpCtx::secretKey() {
  size_t Size;
  assuming(EVP_PKEY_get_raw_private_key(Ctx.get(), nullptr, &Size));

  std::vector<uint8_t> Res;
  Res.reserve(Size);
  Res.resize(Size);
  assuming(EVP_PKEY_get_raw_private_key(Ctx.get(), Res.data(), &Size));

  return X25519SKCtx::import(Res, __WASI_SECRETKEY_ENCODING_RAW);
}

WasiCryptoExpect<X25519KpCtx> X25519KpCtx::generate(KxAlgorithm) {
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx{
      EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr)};
  assuming(Ctx);
  assuming(EVP_PKEY_keygen_init(Ctx.get()));

  EVP_PKEY *PKey = nullptr;
  assuming(EVP_PKEY_keygen(Ctx.get(), &PKey));

  return X25519KpCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{PKey}};
}

// N/A
WasiCryptoExpect<X25519KpCtx> X25519KpCtx::import() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
