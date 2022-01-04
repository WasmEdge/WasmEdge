// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/key_exchange/alg.h"
#include "host/wasi_crypto/wrapper/openssl.h"
#include "openssl/evp.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class X25519PKCtx {
public:
  X25519PKCtx(OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Ctx)
      : Pk(std::move(Ctx)) {}

  // Raw
  static WasiCryptoExpect<X25519PKCtx>
  import(Span<uint8_t const> Raw, __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>> exportData();

  inline static __wasi_size_t Len = 32;

private:
  friend class X25519SKCtx;
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Pk;
};

class X25519SKCtx {
public:
  X25519SKCtx(OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Ctx)
      : Sk(std::move(Ctx)) {}

  // Raw
  static WasiCryptoExpect<X25519SKCtx>
  import(Span<uint8_t const> Raw, __wasi_secretkey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>> exportData();

  WasiCryptoExpect<X25519PKCtx> producePublicKey();

  WasiCryptoExpect<std::vector<uint8_t>> dh(X25519PKCtx &Pk);

  inline static __wasi_size_t Len = 32;

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Sk;
};

class X25519KpCtx {
public:
  X25519KpCtx(OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Ctx)
      : Ctx(std::move(Ctx)) {}

  WasiCryptoExpect<X25519PKCtx> publicKey();

  WasiCryptoExpect<X25519SKCtx> secretKey();

  static WasiCryptoExpect<X25519KpCtx> generate(KxAlgorithm Alg);

  static WasiCryptoExpect<X25519KpCtx> import();

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Ctx;
};
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
