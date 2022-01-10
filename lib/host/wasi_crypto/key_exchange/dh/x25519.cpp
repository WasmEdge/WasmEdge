// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/dh/x25519.h"
#include <openssl/x509.h>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Kx {

WasiCryptoExpect<std::unique_ptr<X25519PublicKey>>
X25519PublicKey::import(Span<const uint8_t> Encoded,
                        __wasi_publickey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    EVP_PKEY *Pk = nullptr;
    Pk = EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, nullptr, Encoded.data(),
                                     Encoded.size());
    opensslAssuming(Pk);
    return std::make_unique<X25519PublicKey>(Pk);
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<std::vector<uint8_t>>
X25519PublicKey::exportData(__wasi_publickey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    size_t Size;
    opensslAssuming(EVP_PKEY_get_raw_public_key(Pk.get(), nullptr, &Size));
    std::vector<uint8_t> Res(Size);
    opensslAssuming(EVP_PKEY_get_raw_public_key(Pk.get(), Res.data(), &Size));
    return Res;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<void> X25519PublicKey::verify() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::unique_ptr<X25519SecretKey>>
X25519SecretKey::import(Span<const uint8_t> Encoded,
                        __wasi_secretkey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW: {
    EVP_PKEY *Sk = EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, nullptr,
                                                Encoded.data(), Encoded.size());
    opensslAssuming(Sk);
    return std::make_unique<X25519SecretKey>(Sk);
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<std::vector<uint8_t>>
X25519SecretKey::exportData(__wasi_secretkey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW: {
    size_t Size;
    opensslAssuming(EVP_PKEY_get_raw_private_key(Sk.get(), nullptr, &Size));

    std::vector<uint8_t> Res(Size);
    opensslAssuming(EVP_PKEY_get_raw_private_key(Sk.get(), Res.data(), &Size));
    return Res;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<std::unique_ptr<PublicKey>> X25519SecretKey::publicKey() {
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> PCtx{
      EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr)};
  opensslAssuming(PCtx);
  opensslAssuming(EVP_PKEY_keygen_init(PCtx.get()));

  EVP_PKEY *P = nullptr;
  opensslAssuming(EVP_PKEY_keygen(PCtx.get(), &P));

  return std::make_unique<X25519PublicKey>(P);
}

WasiCryptoExpect<std::vector<uint8_t>>
X25519SecretKey::dh(std::shared_ptr<PublicKey> Pk) {
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx{
      EVP_PKEY_CTX_new(Sk.get(), nullptr)};
  opensslAssuming(Ctx);
  opensslAssuming(EVP_PKEY_derive_init(Ctx.get()));

  auto Encoded = Pk->exportData(__WASI_PUBLICKEY_ENCODING_RAW);
  if (!Encoded) {
    return WasiCryptoUnexpect(Encoded);
  }

  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> NewPK{EVP_PKEY_new_raw_public_key(
      EVP_PKEY_X25519, nullptr, Encoded->data(), Encoded->size())};
  ensureOrReturn(NewPK, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  // normally peer will be a public key
  opensslAssuming(EVP_PKEY_derive_set_peer(Ctx.get(), NewPK.get()));

  size_t Size;
  ensureOrReturn(EVP_PKEY_derive(Ctx.get(), nullptr, &Size),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);

  std::vector<uint8_t> Res(Size);
  ensureOrReturn(EVP_PKEY_derive(Ctx.get(), Res.data(), &Size),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return Res;
}

WasiCryptoExpect<std::unique_ptr<KeyPair>>
X25519KeyPair::Builder::generate(std::shared_ptr<Options>) {
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ct{
      EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr)};
  opensslAssuming(Ct);
  opensslAssuming(EVP_PKEY_keygen_init(Ct.get()));

  EVP_PKEY *PKey = nullptr;
  opensslAssuming(EVP_PKEY_keygen(Ct.get(), &PKey));

  return std::make_unique<X25519KeyPair>(PKey);
}

WasiCryptoExpect<std::unique_ptr<KeyPair>>
X25519KeyPair::Builder::import(Span<const uint8_t>,
                               __wasi_keypair_encoding_e_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<void> X25519KeyPair::verify() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::unique_ptr<PublicKey>> X25519KeyPair::publicKey() {
  size_t Size;
  opensslAssuming(EVP_PKEY_get_raw_public_key(Ctx.get(), nullptr, &Size));

  std::vector<uint8_t> Res(Size);
  opensslAssuming(EVP_PKEY_get_raw_public_key(Ctx.get(), Res.data(), &Size));

  EVP_PKEY *NewPublicKey = EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, nullptr,
                                                       Res.data(), Res.size());

  return std::make_unique<X25519PublicKey>(NewPublicKey);
}

WasiCryptoExpect<std::unique_ptr<SecretKey>> X25519KeyPair::secretKey() {
  size_t Size;
  opensslAssuming(EVP_PKEY_get_raw_private_key(Ctx.get(), nullptr, &Size));

  std::vector<uint8_t> Res(Size);
  opensslAssuming(EVP_PKEY_get_raw_private_key(Ctx.get(), Res.data(), &Size));

  EVP_PKEY *NewPrivateKey = EVP_PKEY_new_raw_private_key(
      EVP_PKEY_X25519, nullptr, Res.data(), Res.size());

  return std::make_unique<X25519SecretKey>(NewPrivateKey);
}

WasiCryptoExpect<std::vector<uint8_t>>
X25519KeyPair::exportData(__wasi_keypair_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    size_t Size;
    opensslAssuming(EVP_PKEY_get_raw_private_key(Ctx.get(), nullptr, &Size));

    std::vector<uint8_t> Res(Size);
    opensslAssuming(EVP_PKEY_get_raw_private_key(Ctx.get(), Res.data(), &Size));
    return Res;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

} // namespace Kx
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
