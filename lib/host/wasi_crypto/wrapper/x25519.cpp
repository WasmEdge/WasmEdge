// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/wrapper/x25519.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<X25519> X25519::make() {
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_free> Ctx{
      EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr)};
  if (Ctx == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  if (1 != EVP_PKEY_keygen_init(Ctx.get())) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
  }
  return X25519{std::move(Ctx)};
}

X25519::X25519(OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_free> Ctx)
    : Ctx(std::move(Ctx)) {}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
