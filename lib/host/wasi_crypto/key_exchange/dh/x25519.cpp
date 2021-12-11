// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/dh/x25519.h"
#include "host/wasi_crypto/wrapper/random.h"
#include "host/wasi_crypto/wrapper/x25519.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<KxPublicKey>
X25519PublicKey::Builder::fromRaw(Span<const uint8_t> Raw) {
  if (Raw.size() != X25519PK::Len) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }

  auto Pk = import(Alg, Raw);
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  return KxPublicKey{std::make_unique<X25519PublicKey>(std::move(*Pk))};
}

WasiCryptoExpect<X25519PublicKey>
X25519PublicKey::import(KxAlgorithm Alg, Span<const uint8_t> Raw) {
  auto Res = X25519PK::import(Raw);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return X25519PublicKey{Alg, std::move(*Res)};
}

WasiCryptoExpect<std::vector<uint8_t>> X25519PublicKey::asRaw() {
  return Ctx.asRaw();
}

WasiCryptoExpect<void> X25519PublicKey::verify() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<KxSecretKey>
X25519SecretKey::Builder::fromRaw(Span<const uint8_t> Raw) {
  if (Raw.size() != X25519SK::Len) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }

  auto Sk = import(Alg, Raw);
  if (!Sk) {
    return WasiCryptoUnexpect(Sk);
  }

  return KxSecretKey{std::make_unique<X25519SecretKey>(std::move(*Sk))};
}

WasiCryptoExpect<X25519SecretKey>
X25519SecretKey::import(KxAlgorithm Alg, Span<const uint8_t> Raw) {
  auto Res = X25519SK::import(Raw);
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

WasiCryptoExpect<__wasi_size_t> X25519SecretKey::len() { return X25519SK::Len; }

WasiCryptoExpect<Span<const uint8_t>> X25519SecretKey::asRaw() {
  return Ctx.asRaw();
}

WasiCryptoExpect<std::vector<uint8_t>>
X25519SecretKey::dh(std::unique_ptr<KxPublicKey::Base> &KxPk) {
  auto *Res = dynamic_cast<X25519PublicKey *>(KxPk.get());
  if (Res == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }

  // TODO: may not friend?
  return Ctx.dk(Res->Ctx);
}

WasiCryptoExpect<X25519PublicKey> X25519SecretKey::producePublicKey() {
  auto Res = Ctx.producePublicKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return X25519PublicKey{Alg, std::move(*Res)};
}

WasiCryptoExpect<KxKeyPair>
X25519KeyPair::Builder::generate(std::optional<KxOptions> Options) {
  CryptoRandom Rng;
  std::vector<uint8_t> SkRaw(X25519SK::Len, 0);
  Rng.fill(SkRaw);

  auto KpSk = X25519SecretKey::import(Alg, SkRaw);
  if (!KpSk) {
    return WasiCryptoUnexpect(KpSk);
  }

  auto KpPk = KpSk->producePublicKey();
  if (!KpPk) {
    return WasiCryptoUnexpect(KpPk);
  }

  //  auto Kp = X25519KeyPair{Alg, *Pk, *Sk};
  return KxKeyPair{
      std::make_unique<X25519KeyPair>(Alg, std::move(*KpPk), std::move(*KpSk))};
}

WasiCryptoExpect<void> X25519KeyPair::verify() {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<KxPublicKey> X25519KeyPair::publicKey() {
  return KxPublicKey{std::make_unique<X25519PublicKey>(std::move(Pk))};
}

WasiCryptoExpect<KxSecretKey> X25519KeyPair::secretKey() {
  return KxSecretKey{std::make_unique<X25519SecretKey>(std::move(Sk))};
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
