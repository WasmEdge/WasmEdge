// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/kx/dh/x25519.h"
#include "host/wasi_crypto/utils/optional.h"
#include <memory>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {
namespace {
inline const size_t PkSize = 32;
inline const size_t SkSize = 32;
inline const size_t KpSize = 64;
inline const size_t SharedSecretSize = 32;
} // namespace

WasiCryptoExpect<std::vector<uint8_t>> X25519::PublicKey::exportData(
    __wasi_publickey_encoding_e_t Encoding) const noexcept {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    std::vector<uint8_t> Res(PkSize);

    size_t Size = PkSize;
    opensslAssuming(EVP_PKEY_get_raw_public_key(Ctx.get(), Res.data(), &Size));
    ensureOrReturn(Size == PkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

    return Res;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<void> X25519::PublicKey::verify() const noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::vector<uint8_t>> X25519::SecretKey::exportData(
    __wasi_secretkey_encoding_e_t Encoding) const noexcept {
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW: {
    std::vector<uint8_t> Res(SkSize);

    size_t Size = SkSize;
    opensslAssuming(EVP_PKEY_get_raw_private_key(Ctx.get(), Res.data(), &Size));
    ensureOrReturn(Size == SkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

    return Res;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<X25519::PublicKey>
X25519::SecretKey::publicKey() const noexcept {
  std::array<uint8_t, SkSize> Res;

  size_t Size = SkSize;
  opensslAssuming(EVP_PKEY_get_raw_public_key(Ctx.get(), Res.data(), &Size));
  ensureOrReturn(Size == SkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  EvpPkeyPtr Pk{EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, nullptr,
                                            Res.data(), Res.size())};
  return Pk;
}

WasiCryptoExpect<std::vector<uint8_t>>
X25519::SecretKey::dh(PublicKey &Pk) noexcept {
  EvpPkeyCtxPtr SkCtx{EVP_PKEY_CTX_new(Ctx.get(), nullptr)};
  opensslAssuming(EVP_PKEY_derive_init(SkCtx.get()));

  // set peer key
  opensslAssuming(EVP_PKEY_derive_set_peer(SkCtx.get(), Pk.raw().get()));

  // generate shared secret
  std::vector<uint8_t> Res(SharedSecretSize);
  size_t Size = SharedSecretSize;
  ensureOrReturn(EVP_PKEY_derive(SkCtx.get(), Res.data(), &Size),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(Size == SharedSecretSize,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return Res;
}

WasiCryptoExpect<X25519::KeyPair>
X25519::SecretKey::toKeyPair(PublicKey &) noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<X25519::PublicKey>
X25519::KeyPair::publicKey() const noexcept {
  std::array<uint8_t, PkSize> Res;

  size_t Size = PkSize;
  opensslAssuming(EVP_PKEY_get_raw_public_key(Ctx.get(), Res.data(), &Size));
  ensureOrReturn(Size == PkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  EvpPkeyPtr Pk{EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, nullptr,
                                            Res.data(), Res.size())};
  return Pk;
}

WasiCryptoExpect<X25519::SecretKey>
X25519::KeyPair::secretKey() const noexcept {
  std::array<uint8_t, SkSize> Res;

  size_t Size = SkSize;
  opensslAssuming(EVP_PKEY_get_raw_private_key(Ctx.get(), Res.data(), &Size));
  ensureOrReturn(Size == SkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  EvpPkeyPtr Sk{EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, nullptr,
                                             Res.data(), Res.size())};
  return Sk;
}

WasiCryptoExpect<std::vector<uint8_t>> X25519::KeyPair::exportData(
    __wasi_keypair_encoding_e_t Encoding) const noexcept {
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    std::vector<uint8_t> Res(KpSize);

    size_t Size = PkSize;
    opensslAssuming(EVP_PKEY_get_raw_public_key(Ctx.get(), Res.data(), &Size));
    ensureOrReturn(Size == PkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

    Size = SkSize;
    opensslAssuming(
        EVP_PKEY_get_raw_private_key(Ctx.get(), Res.data() + PkSize, &Size));
    ensureOrReturn(Size == SkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
    return Res;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<X25519::PublicKey>
X25519::PublicKey::import(Span<const uint8_t> Encoded,
                          __wasi_publickey_encoding_e_t Encoding) noexcept {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    EvpPkeyPtr Pk{EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, nullptr,
                                              Encoded.data(), Encoded.size())};
    ensureOrReturn(Pk, __WASI_CRYPTO_ERRNO_INVALID_KEY);
    return Pk;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<X25519::SecretKey>
X25519::SecretKey::import(Span<const uint8_t> Encoded,
                          __wasi_secretkey_encoding_e_t Encoding) noexcept {
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW: {
    EvpPkeyPtr Sk{EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, nullptr,
                                               Encoded.data(), Encoded.size())};
    ensureOrReturn(Sk, __WASI_CRYPTO_ERRNO_INVALID_KEY);
    return Sk;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<X25519::KeyPair>
X25519::KeyPair::generate(OptionalRef<Options>) noexcept {
  EvpPkeyCtxPtr Ctx{EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr)};
  opensslAssuming(EVP_PKEY_keygen_init(Ctx.get()));

  EVP_PKEY *Kp = nullptr;
  opensslAssuming(EVP_PKEY_keygen(Ctx.get(), &Kp));

  return EvpPkeyPtr{Kp};
}

WasiCryptoExpect<X25519::KeyPair>
X25519::KeyPair::import(Span<const uint8_t> Encoded,
                        __wasi_keypair_encoding_e_t Encoding) noexcept {
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    ensureOrReturn(Encoded.size() == KpSize, __WASI_CRYPTO_ERRNO_INVALID_KEY);
    // no found way to set the public key in openssl, just auto generate.
    EvpPkeyPtr Sk{EVP_PKEY_new_raw_private_key(
        EVP_PKEY_X25519, nullptr, Encoded.data() + PkSize, SkSize)};
    ensureOrReturn(Sk, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

    return Sk;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
