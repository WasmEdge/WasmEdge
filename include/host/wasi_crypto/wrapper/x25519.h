// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/wrapper/openssl.h"
#include "openssl/evp.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
class A {
  void t() {
    EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, nullptr, );
    EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, nullptr)
  }
};

class X25519 {
public:
  static WasiCryptoExpect<X25519> make();

  inline static __wasi_size_t PkLen = 32;

  inline static __wasi_size_t SkLen = 32;

private:
  X25519(OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_free> Ctx);

  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_free> Ctx;
};
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
