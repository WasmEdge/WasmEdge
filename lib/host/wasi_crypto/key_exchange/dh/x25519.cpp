// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/dh/x25519.h"
#include "host/wasi_crypto/wrapper/random.h"
#include "host/wasi_crypto/wrapper/x25519.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<X25519PublicKey>
X25519PublicKey::make(KxAlgorithm Alg, Span<const uint8_t> Raw) {
  X25519::make();
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<X25519PublicKey>();
}

KxAlgorithm X25519PublicKey::alg() { return Alg; }

WasiCryptoExpect<__wasi_size_t> X25519PublicKey::len() { return X25519::PkLen; }

WasiCryptoExpect<Span<const uint8_t>> X25519PublicKey::asRaw() {}

WasiCryptoExpect<void> X25519PublicKey::verify() {
  return KxPublicKeyBase::verify();
}

WasiCryptoExpect<KxPublicKey>
X25519PublicKeyBuilder::fromRaw(Span<const uint8_t> Raw) {
  if (Raw.size() != X25519::PkLen) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }

  auto Pk = X25519PublicKey::make(Alg, Raw);
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  return KxPublicKey{std::make_unique<X25519PublicKey>(*Pk)};
}

WasiCryptoExpect<KxSecretKey>
X25519SecretKeyBuilder::fromRaw(Span<const uint8_t> Raw) {
  if (Raw.size() != X25519::SkLen) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }

  auto Sk = X25519SecretKey::make(Alg, Raw);
  if (!Sk) {
    return WasiCryptoUnexpect(Sk);
  }

  return KxSecretKey{std::make_unique<X25519SecretKey>(*Sk)};
}

WasiCryptoExpect<X25519SecretKey>
X25519SecretKey::make(KxAlgorithm Alg, Span<const uint8_t> Raw) {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<X25519SecretKey>();
}

WasiCryptoExpect<X25519PublicKey> X25519SecretKey::producePublicKey() {}

WasiCryptoExpect<KxPublicKey> X25519SecretKey::publicKey() {
  auto Res = producePublicKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return KxPublicKey{std::make_unique<X25519PublicKey>(*Res)};
}

KxAlgorithm X25519SecretKey::alg() { return Alg; }

WasiCryptoExpect<__wasi_size_t> X25519SecretKey::len() { return X25519::SkLen; }

WasiCryptoExpect<Span<const uint8_t>> X25519SecretKey::asRaw() {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<Span<const uint8_t>>();
}

WasiCryptoExpect<std::vector<uint8_t>> X25519SecretKey::dh(KxPublicKey &KxPk) {
  KxPk.inner()->locked([](std::unique_ptr<KxPublicKeyBase> &Key) {
    auto *Res = dynamic_cast<X25519PublicKey *>(Key.get());
    if (Res == nullptr) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
    }
    Res->
  });
}

X25519KeyPairBuilder::X25519KeyPairBuilder(KxAlgorithm Alg) : Alg(Alg) {}

WasiCryptoExpect<KxKeyPair>
X25519KeyPairBuilder::generate(std::optional<KxOptions> Options) {
  CryptoRandom Rng;
  std::vector<uint8_t> SkRaw(X25519::SkLen, 0);
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
  return KxKeyPair{std::make_unique<X25519KeyPair>(Alg, *Pk, *Sk)};
}

X25519KeyPair::X25519KeyPair(KxAlgorithm Alg, X25519PublicKey PublicKey,
                             X25519SecretKey SecretKey)
    : Alg(Alg), PublicKey(PublicKey), SecretKey(SecretKey) {}

KxAlgorithm X25519KeyPair::alg() { return Alg; }

WasiCryptoExpect<void> X25519KeyPair::verify() {
  return KxKeyPairBase::verify();
}

WasiCryptoExpect<KxPublicKey> X25519KeyPair::publicKey() {
  return KxPublicKey{std::make_unique<X25519PublicKey>(PublicKey)};
}

WasiCryptoExpect<KxSecretKey> X25519KeyPair::secretKey() {
  return KxSecretKey{std::make_unique<X25519SecretKey>(SecretKey)};
}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
