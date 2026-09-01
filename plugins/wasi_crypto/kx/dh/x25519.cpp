// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "kx/dh/x25519.h"

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
    opensslCheck(EVP_PKEY_get_raw_public_key(Ctx.get(), Res.data(), &Size));
    ensureOrReturn(Size == PkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

    return Res;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<void> X25519::PublicKey::verify() const noexcept {
  static constexpr uint8_t LowOrderPoints[][32] = {
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
      {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
      {0xe0, 0xeb, 0x7a, 0x7c, 0x3b, 0x41, 0xb8, 0xae, 0x16, 0x56, 0xe3,
       0xfa, 0xf1, 0x9f, 0xc4, 0x6a, 0xda, 0x09, 0x8d, 0xed, 0xb6, 0x1e,
       0x80, 0x7f, 0xef, 0x27, 0x70, 0x55, 0xde, 0x86, 0x10, 0x7f},
      {0x5f, 0x9c, 0x95, 0xbc, 0xa2, 0xb8, 0x8d, 0x16, 0x56, 0xfa, 0x62,
       0x6d, 0x00, 0x01, 0x5f, 0xa3, 0x02, 0x4c, 0xba, 0x5f, 0x19, 0xb0,
       0x70, 0xaa, 0x65, 0x24, 0x3a, 0x47, 0x04, 0x73, 0x15, 0x7f},
      {0xec, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f},
      {0xed, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f},
      {0xee, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f}};

  std::vector<uint8_t> RawPk(PkSize);
  size_t Size = PkSize;
  opensslCheck(EVP_PKEY_get_raw_public_key(Ctx.get(), RawPk.data(), &Size));
  ensureOrReturn(Size == PkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  RawPk[31] &= 0x7f;

  for (const auto &Pt : LowOrderPoints) {
    if (std::equal(RawPk.begin(), RawPk.end(), Pt)) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
    }
  }

  EvpPkeyCtxPtr CheckCtx{EVP_PKEY_CTX_new(Ctx.get(), nullptr)};
  ensureOrReturn(CheckCtx, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  int Rc = EVP_PKEY_public_check(CheckCtx.get());
  if (Rc < 0 && Rc != -2) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  }
  if (Rc == 0) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }
  return {};
}

WasiCryptoExpect<SecretVec> X25519::SecretKey::exportData(
    __wasi_secretkey_encoding_e_t Encoding) const noexcept {
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW: {
    SecretVec Res(SkSize);

    size_t Size = SkSize;
    opensslCheck(EVP_PKEY_get_raw_private_key(Ctx.get(), Res.data(), &Size));
    ensureOrReturn(Size == SkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

    return Res;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<X25519::PublicKey>
X25519::SecretKey::publicKey() const noexcept {
  // Since the inner is always `const`, we just increase the ref count.
  return Ctx;
}

WasiCryptoExpect<SecretVec>
X25519::SecretKey::dh(const PublicKey &Pk) const noexcept {
  EvpPkeyCtxPtr SkCtx{EVP_PKEY_CTX_new(Ctx.get(), nullptr)};
  opensslCheck(EVP_PKEY_derive_init(SkCtx.get()));

  // Set peer key.
  opensslCheck(EVP_PKEY_derive_set_peer(SkCtx.get(), Pk.raw().get()));

  // Generate shared secret.
  SecretVec Res(SharedSecretSize);
  size_t Size = SharedSecretSize;
  ensureOrReturn(EVP_PKEY_derive(SkCtx.get(), Res.data(), &Size),
                 __WASI_CRYPTO_ERRNO_INVALID_KEY);
  ensureOrReturn(Size == SharedSecretSize,
                 __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return Res;
}

WasiCryptoExpect<X25519::KeyPair>
X25519::SecretKey::toKeyPair(const PublicKey &Pk) const noexcept {
  int Rc = EVP_PKEY_cmp(Ctx.get(), Pk.raw().get());
  ensureOrReturn(Rc >= 0, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  ensureOrReturn(Rc == 1, __WASI_CRYPTO_ERRNO_INCOMPATIBLE_KEYS);
  return Ctx;
}

WasiCryptoExpect<X25519::PublicKey>
X25519::KeyPair::publicKey() const noexcept {
  // Since the inner is always `const`, we just increase the ref count.
  return Ctx;
}

WasiCryptoExpect<X25519::SecretKey>
X25519::KeyPair::secretKey() const noexcept {
  // Since the inner is always `const`, we just increase the ref count.
  return Ctx;
}

WasiCryptoExpect<SecretVec> X25519::KeyPair::exportData(
    __wasi_keypair_encoding_e_t Encoding) const noexcept {
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    SecretVec Res(KpSize);

    size_t Size = PkSize;
    opensslCheck(EVP_PKEY_get_raw_public_key(Ctx.get(), Res.data(), &Size));
    ensureOrReturn(Size == PkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

    Size = SkSize;
    opensslCheck(
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
X25519::KeyPair::generate(OptionalRef<const Options>) noexcept {
  EvpPkeyCtxPtr Ctx{EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr)};
  opensslCheck(EVP_PKEY_keygen_init(Ctx.get()));

  EVP_PKEY *Kp = nullptr;
  opensslCheck(EVP_PKEY_keygen(Ctx.get(), &Kp));

  return EvpPkeyPtr{Kp};
}

WasiCryptoExpect<X25519::KeyPair>
X25519::KeyPair::import(Span<const uint8_t> Encoded,
                        __wasi_keypair_encoding_e_t Encoding) noexcept {
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    ensureOrReturn(Encoded.size() == KpSize, __WASI_CRYPTO_ERRNO_INVALID_KEY);
    // PublicKey can auto generate from SecretKey.
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
