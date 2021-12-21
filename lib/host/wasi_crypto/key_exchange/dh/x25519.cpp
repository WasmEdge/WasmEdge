// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/dh/x25519.h"
#include "host/wasi_crypto/wrapper/random.h"
#include "host/wasi_crypto/wrapper/x25519.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<KxPublicKey>
X25519PublicKey::Builder::import(Span<uint8_t const> Raw,
                                 __wasi_publickey_encoding_e_t Encoding) {
  if (Raw.size() != X25519PKCtx::Len) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }

  auto Pk = X25519PublicKey::import(Alg, Raw, Encoding);
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  return KxPublicKey{std::make_unique<X25519PublicKey>(std::move(*Pk))};
}

WasiCryptoExpect<X25519PublicKey>
X25519PublicKey::import(KxAlgorithm Alg, Span<const uint8_t> Raw,
                        __wasi_publickey_encoding_e_t /*TODO:Encoding*/) {
  auto Res = X25519PKCtx::import(Raw);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return X25519PublicKey{Alg, std::move(*Res)};
}

WasiCryptoExpect<std::vector<uint8_t>> X25519PublicKey::asRef() {
  return Ctx.asRaw();
}

WasiCryptoExpect<void> X25519PublicKey::verify() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<KxSecretKey> X25519SecretKey::Builder::import(
    Span<const uint8_t> Raw, __wasi_secretkey_encoding_e_t /*TODO:Encoding*/) {
  if (Raw.size() != X25519SKCtx::Len) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }

  auto Res = X25519SKCtx::import(Raw);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return KxSecretKey{std::make_unique<X25519SecretKey>(Alg, std::move(*Res))};
}

WasiCryptoExpect<X25519SecretKey>
X25519SecretKey::import(KxAlgorithm Alg, Span<const uint8_t> Raw,
                        __wasi_secretkey_encoding_e_t /*TODO:Encoding*/) {
  auto Res = X25519SKCtx::import(Raw);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return X25519SecretKey{Alg, std::move(*Res)};
}

WasiCryptoExpect<KxPublicKey> X25519SecretKey::publicKey() {
  auto Res = producePublicKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return KxPublicKey{std::make_unique<X25519PublicKey>(std::move(*Res))};
}

WasiCryptoExpect<__wasi_size_t> X25519SecretKey::len() {
  return X25519SKCtx::Len;
}

WasiCryptoExpect<Span<const uint8_t>> X25519SecretKey::asRef() {
  return Ctx.asRaw();
}

WasiCryptoExpect<std::vector<uint8_t>>
X25519SecretKey::dh(std::unique_ptr<KxPublicKey::Base> &KxPk) {
  auto *Res = dynamic_cast<X25519PublicKey *>(KxPk.get());
  if (Res == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }

  // TODO: may not friend?
  return Ctx.dh(Res->Ctx);
}

WasiCryptoExpect<X25519PublicKey> X25519SecretKey::producePublicKey() {
  auto Res = Ctx.producePublicKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return X25519PublicKey{Alg, std::move(*Res)};
}

WasiCryptoExpect<KxKeyPair>
X25519KeyPair::Builder::generate(std::optional<KxOptions> /*TODO:Options*/) {
  auto KpCtx = X25519KpCtx::generate(Alg);
  if (!KpCtx) {
    return WasiCryptoUnexpect(KpCtx);
  }

  //  auto Kp = X25519KeyPair{Alg, *Pk, *Sk};
  return KxKeyPair{std::make_unique<X25519KeyPair>(Alg, std::move(*KpCtx))};
}

WasiCryptoExpect<KxKeyPair> X25519KeyPair::import(KxAlgorithm,
                                                  Span<const uint8_t>,
                                                  __wasi_keypair_encoding_e_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<void> X25519KeyPair::verify() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<KxPublicKey> X25519KeyPair::publicKey() {
  auto Res = Kp.publicKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return KxPublicKey{std::make_unique<X25519PublicKey>(Alg, std::move(*Res))};
}

WasiCryptoExpect<KxSecretKey> X25519KeyPair::secretKey() {
  auto Res = Kp.secretKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return KxSecretKey{std::make_unique<X25519SecretKey>(Alg, std::move(*Res))};
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
