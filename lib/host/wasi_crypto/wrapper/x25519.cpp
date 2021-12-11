// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/wrapper/x25519.h"
#include "host/wasi_crypto/key_exchange/dh/x25519.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
WasiCryptoExpect<X25519PK> X25519PK::import(Span<const uint8_t> Raw) {
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Pk{EVP_PKEY_new_raw_public_key(
      EVP_PKEY_X25519, nullptr, Raw.data(), Raw.size())};
  if (Pk == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  return X25519PK{std::move(Pk)};
}

WasiCryptoExpect<std::vector<uint8_t>> X25519PK::asRaw() {
  size_t Size;
  if (1 != EVP_PKEY_get_raw_public_key(Pk.get(), nullptr, &Size)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  std::vector<uint8_t> Res(Size);
  if (1 != EVP_PKEY_get_raw_public_key(Pk.get(), Res.data(), &Size)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  return Res;
}

WasiCryptoExpect<X25519SK> X25519SK::import(Span<const uint8_t> Raw) {
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Sk{EVP_PKEY_new_raw_private_key(
      EVP_PKEY_X25519, nullptr, Raw.data(), Raw.size())};
  if (Sk == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  return X25519SK{std::move(Sk)};
}

WasiCryptoExpect<std::vector<uint8_t>> X25519SK::asRaw() {
  size_t Size;
  if (1 != EVP_PKEY_get_raw_private_key(Sk.get(), nullptr, &Size)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  std::vector<uint8_t> Res(Size);
  if (1 != EVP_PKEY_get_raw_private_key(Sk.get(), Res.data(), &Size)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  return Res;
}

WasiCryptoExpect<X25519PK> X25519SK::producePublicKey() {
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Pctx{
      EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr)};
  EVP_PKEY_keygen_init(Pctx.get());

  EVP_PKEY *P = nullptr;
  EVP_PKEY_keygen(Pctx.get(), &P);
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Pkey{P};
  return X25519PK{std::move(Pkey)};
}

WasiCryptoExpect<std::vector<uint8_t>> X25519SK::dk(X25519PK &Pk) {
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx{
      EVP_PKEY_CTX_new(Pk.Pk.get(), nullptr)};
  if (Ctx == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  if (EVP_PKEY_derive_set_peer(Ctx.get(), Sk.get()) <= 0) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  size_t Size;
  if (1 != EVP_PKEY_derive(Ctx.get(), nullptr, &Size)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }

  std::vector<uint8_t> Res(Size);
  if (1 != EVP_PKEY_derive(Ctx.get(), Res.data(), &Size)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  return Res;
}

WasiCryptoExpect<X25519Kp> X25519Kp::make() {
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_free> Ctx{
      EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr)};
  if (Ctx == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  if (1 != EVP_PKEY_keygen_init(Ctx.get())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  return X25519Kp{std::move(Ctx)};
}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
