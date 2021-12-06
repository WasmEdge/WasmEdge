// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/dh/x25519.h"
#include "host/wasi_crypto/wrapper/random.h"
#include "host/wasi_crypto/wrapper/x25519.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<KxPublicKey>
X25519PublicKeyBuilder::fromRaw(Span<const uint8_t> Raw) {
  if (Raw.size() != X25519PK::Len) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }

  auto Pk = X25519PublicKey::make(Alg, Raw);
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  return KxPublicKey{std::make_unique<X25519PublicKey>(std::move(*Pk))};
}

WasiCryptoExpect<X25519PublicKey>
X25519PublicKey::make(KxAlgorithm Alg, Span<const uint8_t> Raw) {
  auto Res = X25519PK::make(Raw);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return X25519PublicKey{Alg, std::move(*Res)};
}

KxAlgorithm X25519PublicKey::alg() { return Alg; }

WasiCryptoExpect<__wasi_size_t> X25519PublicKey::len() { return X25519PK::Len; }

WasiCryptoExpect<Span<const uint8_t>> X25519PublicKey::asRaw() {
  return Ctx.asRaw();
}

WasiCryptoExpect<void> X25519PublicKey::verify() {}

X25519PublicKey::X25519PublicKey(KxAlgorithm Alg, X25519PK Ctx)
    : Alg(Alg), Ctx(std::move(Ctx)) {}

WasiCryptoExpect<KxSecretKey>
X25519SecretKeyBuilder::fromRaw(Span<const uint8_t> Raw) {
  if (Raw.size() != X25519SK::Len) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }

  auto Sk = X25519SecretKey::make(Alg, Raw);
  if (!Sk) {
    return WasiCryptoUnexpect(Sk);
  }

  return KxSecretKey{std::make_unique<X25519SecretKey>(std::move(*Sk))};
}

WasiCryptoExpect<X25519SecretKey>
X25519SecretKey::make(KxAlgorithm Alg, Span<const uint8_t> Raw) {
  auto Res = X25519SK::make(Raw);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return X25519SecretKey{Alg, std::move(*Res)};
}

WasiCryptoExpect<X25519PublicKey> X25519SecretKey::producePublicKey() {
  auto Res = Ctx.producePublicKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return X25519PublicKey{Alg, std::move(*Res)};
}

WasiCryptoExpect<KxPublicKey> X25519SecretKey::publicKey() {
  auto Res = producePublicKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return KxPublicKey{std::make_unique<X25519PublicKey>(std::move(*Res))};
}

KxAlgorithm X25519SecretKey::alg() { return Alg; }

WasiCryptoExpect<__wasi_size_t> X25519SecretKey::len() { return X25519SK::Len; }

WasiCryptoExpect<Span<const uint8_t>> X25519SecretKey::asRaw() {
  return Ctx.asRaw();
}

WasiCryptoExpect<std::vector<uint8_t>> X25519SecretKey::dh(KxPublicKey &KxPk) {
  KxPk.inner()->locked([](auto &Key) {
    auto *Res = dynamic_cast<X25519PublicKey *>(Key.get());
    if (Res == nullptr) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
    }
    Res->
  });
}

X25519SecretKey::X25519SecretKey(KxAlgorithm Alg, X25519SK Ctx)
    : Alg(Alg), Ctx(std::move(Ctx)) {}

X25519KeyPairBuilder::X25519KeyPairBuilder(KxAlgorithm Alg) : Alg(Alg) {}

WasiCryptoExpect<KxKeyPair>
X25519KeyPairBuilder::generate(std::optional<KxOptions> Options) {
  CryptoRandom Rng;
  std::vector<uint8_t> SkRaw(X25519SK::Len, 0);
  Rng.fill(SkRaw);

  auto Sk = X25519SecretKey::make(Alg, SkRaw);
  if (!Sk) {
    return WasiCryptoUnexpect(Sk);
  }

  auto Pk = Sk->producePublicKey();
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  //  auto Kp = X25519KeyPair{Alg, *Pk, *Sk};
  return KxKeyPair{
      std::make_unique<X25519KeyPair>(Alg, std::move(*Pk), std::move(*Sk))};
}

X25519KeyPair::X25519KeyPair(KxAlgorithm Alg, X25519PublicKey PublicKey,
                             X25519SecretKey SecretKey)
    : Alg(Alg), PublicKey(std::move(PublicKey)),
      SecretKey(std::move(SecretKey)) {}

KxAlgorithm X25519KeyPair::alg() { return Alg; }

WasiCryptoExpect<void> X25519KeyPair::verify() {}

WasiCryptoExpect<KxPublicKey> X25519KeyPair::publicKey() {

  return KxPublicKey{std::make_unique<X25519PublicKey>(PublicKey)};
}

WasiCryptoExpect<KxSecretKey> X25519KeyPair::secretKey() {
  return KxSecretKey{std::make_unique<X25519SecretKey>(SecretKey)};
}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
