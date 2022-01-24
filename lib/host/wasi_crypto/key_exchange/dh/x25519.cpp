// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/dh/x25519.h"

#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/evpwrapper.h"
#include "openssl/x509.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Kx {

WasiCryptoExpect<std::unique_ptr<X25519PublicKey>>
X25519PublicKey::import(Span<const uint8_t> Encoded,
                        __wasi_publickey_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    EvpPkeyPtr Pk{EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, nullptr,
                                              Encoded.data(), Encoded.size())};
    ensureOrReturn(Pk, __WASI_CRYPTO_ERRNO_INVALID_KEY);
    return std::make_unique<X25519PublicKey>(std::move(Pk));
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
    opensslAssuming(EVP_PKEY_get_raw_public_key(Ctx.get(), nullptr, &Size));
    std::vector<uint8_t> Res(Size);
    opensslAssuming(EVP_PKEY_get_raw_public_key(Ctx.get(), Res.data(), &Size));
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
    EvpPkeyPtr Sk{EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, nullptr,
                                               Encoded.data(), Encoded.size())};
    ensureOrReturn(Sk, __WASI_CRYPTO_ERRNO_INVALID_KEY);
    return std::make_unique<X25519SecretKey>(std::move(Sk));
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
    opensslAssuming(EVP_PKEY_get_raw_private_key(Ctx.get(), nullptr, &Size));

    std::vector<uint8_t> Res(Size);
    opensslAssuming(EVP_PKEY_get_raw_private_key(Ctx.get(), Res.data(), &Size));
    return Res;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<std::unique_ptr<PublicKey>> X25519SecretKey::publicKey() {
  size_t Size;
  opensslAssuming(EVP_PKEY_get_raw_public_key(Ctx.get(), nullptr, &Size));

  std::vector<uint8_t> Res(Size);
  opensslAssuming(EVP_PKEY_get_raw_public_key(Ctx.get(), Res.data(), &Size));

  EvpPkeyPtr Pk{EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, nullptr,
                                            Res.data(), Res.size())};
  return std::make_unique<X25519PublicKey>(std::move(Pk));
}

WasiCryptoExpect<std::vector<uint8_t>>
X25519SecretKey::dh(std::shared_ptr<PublicKey> PkKey) {
  EvpPkeyCtxPtr SkCtx{EVP_PKEY_CTX_new(Ctx.get(), nullptr)};
  opensslAssuming(EVP_PKEY_derive_init(SkCtx.get()));

  auto PkEncoded = PkKey->exportData(__WASI_PUBLICKEY_ENCODING_RAW);
  if (!PkEncoded) {
    return WasiCryptoUnexpect(PkEncoded);
  }

  EvpPkeyPtr PK{EVP_PKEY_new_raw_public_key(
      EVP_PKEY_X25519, nullptr, PkEncoded->data(), PkEncoded->size())};
  ensureOrReturn(PK, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  // normally peer will be a public key
  opensslAssuming(EVP_PKEY_derive_set_peer(SkCtx.get(), PK.get()));

  size_t Size;
  ensureOrReturn(EVP_PKEY_derive(SkCtx.get(), nullptr, &Size),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);

  std::vector<uint8_t> Res(Size);
  ensureOrReturn(EVP_PKEY_derive(SkCtx.get(), Res.data(), &Size),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);

  return Res;
}

WasiCryptoExpect<std::unique_ptr<KeyPair>>
X25519KeyPair::Builder::generate(std::shared_ptr<Options>) {
  EvpPkeyCtxPtr Ct{EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr)};
  opensslAssuming(Ct);
  opensslAssuming(EVP_PKEY_keygen_init(Ct.get()));

  EVP_PKEY *PKey = nullptr;
  opensslAssuming(EVP_PKEY_keygen(Ct.get(), &PKey));

  return std::make_unique<X25519KeyPair>(EvpPkeyPtr{PKey});
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

  EvpPkeyPtr Pk{EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, nullptr,
                                            Res.data(), Res.size())};
  return std::make_unique<X25519PublicKey>(std::move(Pk));
}

WasiCryptoExpect<std::unique_ptr<SecretKey>> X25519KeyPair::secretKey() {
  size_t Size;
  opensslAssuming(EVP_PKEY_get_raw_private_key(Ctx.get(), nullptr, &Size));

  std::vector<uint8_t> Res(Size);
  opensslAssuming(EVP_PKEY_get_raw_private_key(Ctx.get(), Res.data(), &Size));

  EvpPkeyPtr Sk{EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, nullptr,
                                             Res.data(), Res.size())};
  return std::make_unique<X25519SecretKey>(std::move(Sk));
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
