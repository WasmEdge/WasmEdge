// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/dh/x25519.h"

#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/evpwrapper.h"
#include "openssl/x509.h"
#include "wasi_crypto/api.hpp"
#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Kx {
namespace {
inline const size_t PkSize = 32;
inline const size_t SkSize = 32;
inline const size_t KpSize = 64;
inline const size_t SharedSecretSize = 32;
} // namespace

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
    std::vector<uint8_t> Res(PkSize);

    size_t Size;
    opensslAssuming(EVP_PKEY_get_raw_public_key(Ctx.get(), Res.data(), &Size));
    ensureOrReturn(Size == PkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

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
    std::vector<uint8_t> Res(SkSize);

    size_t Size;
    opensslAssuming(EVP_PKEY_get_raw_private_key(Ctx.get(), Res.data(), &Size));
    ensureOrReturn(Size == SkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

    return Res;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<std::unique_ptr<PublicKey>> X25519SecretKey::publicKey() {
  std::array<uint8_t, SkSize> Res;

  size_t Size;
  opensslAssuming(EVP_PKEY_get_raw_public_key(Ctx.get(), Res.data(), &Size));
  ensureOrReturn(Size == SkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  EvpPkeyPtr Pk{EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, nullptr,
                                            Res.data(), Res.size())};
  return std::make_unique<X25519PublicKey>(std::move(Pk));
}

WasiCryptoExpect<std::vector<uint8_t>>
X25519SecretKey::dh(std::shared_ptr<PublicKey> PkKey) {
  EvpPkeyCtxPtr SkCtx{EVP_PKEY_CTX_new(Ctx.get(), nullptr)};
  opensslAssuming(EVP_PKEY_derive_init(SkCtx.get()));

  // get raw represent
  auto PkRaw = PkKey->exportData(__WASI_PUBLICKEY_ENCODING_RAW);
  if (!PkRaw) {
    return WasiCryptoUnexpect(PkRaw);
  }

  // get EVP represent
  EvpPkeyPtr PK{EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, nullptr,
                                            PkRaw->data(), PkRaw->size())};
  ensureOrReturn(PK, __WASI_CRYPTO_ERRNO_INVALID_KEY);

  // set peer key
  opensslAssuming(EVP_PKEY_derive_set_peer(SkCtx.get(), PK.get()));

  // generate shared secret
  std::vector<uint8_t> Res(SharedSecretSize);
  size_t Size;
  ensureOrReturn(EVP_PKEY_derive(SkCtx.get(), Res.data(), &Size),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(Size == SharedSecretSize,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return Res;
}

WasiCryptoExpect<std::unique_ptr<KeyPair>>
X25519KeyPair::Builder::generate(std::shared_ptr<Options>) {
  EvpPkeyCtxPtr Ctx{EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr)};
  opensslAssuming(EVP_PKEY_keygen_init(Ctx.get()));

  EVP_PKEY *PKey = nullptr;
  opensslAssuming(EVP_PKEY_keygen(Ctx.get(), &PKey));

  return std::make_unique<X25519KeyPair>(EvpPkeyPtr{PKey});
}

WasiCryptoExpect<std::unique_ptr<KeyPair>>
X25519KeyPair::Builder::import(Span<const uint8_t> Encoded,
                               __wasi_keypair_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    ensureOrReturn(Encoded.size() == KpSize, __WASI_CRYPTO_ERRNO_INVALID_KEY);
    // no way to set the public key in openssl, just auto generate.
    EvpPkeyPtr SkCtx{EVP_PKEY_new_raw_private_key(
        EVP_PKEY_X25519, nullptr, Encoded.data() + PkSize, SkSize)};
    ensureOrReturn(SkCtx, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

    return std::make_unique<X25519KeyPair>(std::move(SkCtx));
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<std::vector<uint8_t>>
X25519KeyPair::exportData(__wasi_keypair_encoding_e_t Encoding) {
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    std::vector<uint8_t> Res(PkSize + SkSize);

    size_t Size;
    opensslAssuming(EVP_PKEY_get_raw_public_key(Ctx.get(), Res.data(), &Size));
    ensureOrReturn(Size == PkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

    opensslAssuming(
        EVP_PKEY_get_raw_private_key(Ctx.get(), Res.data() + PkSize, &Size));
    ensureOrReturn(Size == SkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
    return Res;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<std::unique_ptr<PublicKey>> X25519KeyPair::publicKey() {
  std::array<uint8_t, PkSize> Res;

  size_t Size;
  opensslAssuming(EVP_PKEY_get_raw_public_key(Ctx.get(), Res.data(), &Size));
  ensureOrReturn(Size == PkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  EvpPkeyPtr Pk{EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, nullptr,
                                            Res.data(), Res.size())};
  return std::make_unique<X25519PublicKey>(std::move(Pk));
}

WasiCryptoExpect<std::unique_ptr<SecretKey>> X25519KeyPair::secretKey() {
  std::array<uint8_t, SkSize> Res;

  size_t Size;
  opensslAssuming(EVP_PKEY_get_raw_private_key(Ctx.get(), Res.data(), &Size));
  ensureOrReturn(Size == SkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  EvpPkeyPtr Sk{EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, nullptr,
                                             Res.data(), Res.size())};
  return std::make_unique<X25519SecretKey>(std::move(Sk));
}

} // namespace Kx
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
