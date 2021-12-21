// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/wrapper/x25519.h"
#include "common/errcode.h"
#include <openssl/dh.h>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<X25519PKCtx> X25519PKCtx::import(Span<const uint8_t> Raw) {
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Pk{EVP_PKEY_new_raw_public_key(
      EVP_PKEY_X25519, nullptr, Raw.data(), Raw.size())};
  assuming(Pk);
  return X25519PKCtx{std::move(Pk)};
}

WasiCryptoExpect<std::vector<uint8_t>> X25519PKCtx::asRaw() {
  size_t Size;
  assuming(EVP_PKEY_get_raw_public_key(Pk.get(), nullptr, &Size));

  std::vector<uint8_t> Res;
  Res.reserve(Size);
  Res.resize(Size);
  assuming(EVP_PKEY_get_raw_public_key(Pk.get(), Res.data(), &Size));
  return Res;
}

WasiCryptoExpect<X25519SKCtx> X25519SKCtx::import(Span<const uint8_t> Raw) {
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Sk{EVP_PKEY_new_raw_private_key(
      EVP_PKEY_X25519, nullptr, Raw.data(), Raw.size())};
  assuming(Sk);
  return X25519SKCtx{std::move(Sk)};
}

WasiCryptoExpect<std::vector<uint8_t>> X25519SKCtx::asRaw() {
  size_t Size;
  assuming(EVP_PKEY_get_raw_private_key(Sk.get(), nullptr, &Size));

  std::vector<uint8_t> Res;
  Res.reserve(Size);
  Res.resize(Size);
  assuming(EVP_PKEY_get_raw_private_key(Sk.get(), Res.data(), &Size));
  return Res;
}

WasiCryptoExpect<X25519PKCtx> X25519SKCtx::producePublicKey() {
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> PCtx{
      EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr)};
  assuming(PCtx);
  assuming(EVP_PKEY_keygen_init(PCtx.get()));

  EVP_PKEY *P = nullptr;
  assuming(EVP_PKEY_keygen(PCtx.get(), &P));

  return X25519PKCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{P}};
}

WasiCryptoExpect<std::vector<uint8_t>> X25519SKCtx::dh(X25519PKCtx &Pk) {
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx{
      EVP_PKEY_CTX_new(Sk.get(), nullptr)};
  assuming(Ctx);
  assuming(EVP_PKEY_derive_init(Ctx.get()));

  // normally peer will be a public key
  assuming(EVP_PKEY_derive_set_peer(Ctx.get(), Pk.Pk.get()));

  size_t Size;
  assuming(EVP_PKEY_derive(Ctx.get(), nullptr, &Size));

  std::vector<uint8_t> Res;
  Res.reserve(Size);
  Res.resize(Size);
  assuming(EVP_PKEY_derive(Ctx.get(), Res.data(), &Size));

  return Res;
}

WasiCryptoExpect<X25519PKCtx> X25519KpCtx::publicKey() {
  size_t Size;
  assuming(EVP_PKEY_get_raw_public_key(Ctx.get(), nullptr, &Size));

  std::vector<uint8_t> Res;
  Res.reserve(Size);
  Res.resize(Size);
  assuming(EVP_PKEY_get_raw_public_key(Ctx.get(), Res.data(), &Size));

  return X25519PKCtx::import(Res);
}

WasiCryptoExpect<X25519SKCtx> X25519KpCtx::secretKey() {
  size_t Size;
  assuming(EVP_PKEY_get_raw_private_key(Ctx.get(), nullptr, &Size));

  std::vector<uint8_t> Res;
  Res.reserve(Size);
  Res.resize(Size);
  assuming(EVP_PKEY_get_raw_private_key(Ctx.get(), Res.data(), &Size));

  return X25519SKCtx::import(Res);
}

WasiCryptoExpect<X25519KpCtx> X25519KpCtx::generate(KxAlgorithm) {
  OpenSSLUniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free> Ctx{
      EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr)};
  assuming(Ctx);
  assuming(EVP_PKEY_keygen_init(Ctx.get()));

  EVP_PKEY *PKey = nullptr;
  assuming(EVP_PKEY_keygen(Ctx.get(), &PKey));

  return X25519KpCtx{OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free>{PKey}};
}

WasiCryptoExpect<X25519KpCtx> X25519KpCtx::import() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
