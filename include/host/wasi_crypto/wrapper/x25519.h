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
  static WasiCryptoExpect<X25519PK> make(Span<uint8_t const> Raw);

  X25519PK(const X25519PK &PK);

  Span<const uint8_t> asRaw();

  inline static __wasi_size_t Len = 32;

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> PKCtx;
};

class X25519SK {
public:
  static WasiCryptoExpect<X25519SK> make(Span<uint8_t const> Raw);

  X25519SK(const X25519SK &SK);

  Span<const uint8_t> asRaw();

  WasiCryptoExpect<X25519PK> producePublicKey();

  inline static __wasi_size_t Len = 32;

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> SKCtx;
};

class X25519 {
public:
  static WasiCryptoExpect<X25519> make();

private:
  X25519(OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_free> Ctx);

  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_free> Ctx;
};
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
