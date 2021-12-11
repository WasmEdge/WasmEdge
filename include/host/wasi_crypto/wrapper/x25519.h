// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/wrapper/openssl.h"
#include "openssl/evp.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class X25519PK {
public:
  static WasiCryptoExpect<X25519PK> import(Span<uint8_t const> Raw);

  WasiCryptoExpect<std::vector<uint8_t>> asRaw();

  inline static __wasi_size_t Len = 32;

private:
  friend class X25519SK;
  X25519PK(OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Ctx)
      : Pk(std::move(Ctx)) {}
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Pk;
};

class X25519SK {
public:
  static WasiCryptoExpect<X25519SK> import(Span<uint8_t const> Raw);

  WasiCryptoExpect<std::vector<uint8_t>> asRaw();

  WasiCryptoExpect<X25519PK> producePublicKey();

  WasiCryptoExpect<std::vector<uint8_t>> dk(X25519PK &Pk);

  inline static __wasi_size_t Len = 32;

private:
  X25519SK(OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Ctx)
      : Sk(std::move(Ctx)) {}

  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Sk;
};

class X25519Kp {
public:
  static WasiCryptoExpect<X25519Kp> make();

private:
  X25519Kp(OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx)
      : Ctx(std::move(Ctx)) {}

  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx;
};
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
