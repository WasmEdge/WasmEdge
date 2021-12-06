// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/dh/x25519.h"
#include "host/wasi_crypto/wrapper/random.h"
#include "host/wasi_crypto/wrapper/x25519.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace X25519 {

WasiCryptoExpect<KxPublicKey>
PublicKey::Builder::fromRaw(Span<const uint8_t> Raw) {
  if (Raw.size() != X25519PK::Len) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }

  auto Pk = make(Alg, Raw);
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  return KxPublicKey{std::make_unique<PublicKey>(std::move(*Pk))};
}

WasiCryptoExpect<PublicKey> PublicKey::make(KxAlgorithm Alg,
                                            Span<const uint8_t> Raw) {
  auto Res = X25519PK::make(Raw);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return PublicKey{Alg, std::move(*Res)};
}

WasiCryptoExpect<std::vector<uint8_t>> PublicKey::asRaw() {
  return Ctx.asRaw();
}

WasiCryptoExpect<void> PublicKey::verify() {}

WasiCryptoExpect<KxSecretKey>
SecretKey::Builder::fromRaw(Span<const uint8_t> Raw) {
  if (Raw.size() != X25519SK::Len) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }

  auto Sk = make(Alg, Raw);
  if (!Sk) {
    return WasiCryptoUnexpect(Sk);
  }

  return KxSecretKey{std::make_unique<SecretKey>(std::move(*Sk))};
}

WasiCryptoExpect<SecretKey> SecretKey::make(KxAlgorithm Alg,
                                            Span<const uint8_t> Raw) {
  auto Res = X25519SK::make(Raw);
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return SecretKey{Alg, std::move(*Res)};
}

WasiCryptoExpect<KxPublicKey> SecretKey::publicKey() {
  auto Res = producePublicKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return KxPublicKey{std::make_unique<PublicKey>(std::move(*Res))};
}

WasiCryptoExpect<__wasi_size_t> SecretKey::len() { return X25519SK::Len; }

WasiCryptoExpect<Span<const uint8_t>> SecretKey::asRaw() { return Ctx.asRaw(); }

WasiCryptoExpect<std::vector<uint8_t>>
SecretKey::dh(std::unique_ptr<KxPublicKey::Base> &KxPk) {
  auto *Res = dynamic_cast<PublicKey *>(KxPk.get());
  if (Res == nullptr) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_KEY);
  }

  // TODO: may not friend?
  return Ctx.dk(Res->Ctx);
}

WasiCryptoExpect<PublicKey> SecretKey::producePublicKey() {
  auto Res = Ctx.producePublicKey();
  if (!Res) {
    return WasiCryptoUnexpect(Res);
  }

  return PublicKey{Alg, std::move(*Res)};
}

WasiCryptoExpect<KxKeyPair>
KeyPair::Builder::generate(std::optional<KxOptions> Options) {
  CryptoRandom Rng;
  std::vector<uint8_t> SkRaw(X25519SK::Len, 0);
  Rng.fill(SkRaw);

  auto KpSk = SecretKey::make(Alg, SkRaw);
  if (!KpSk) {
    return WasiCryptoUnexpect(KpSk);
  }

  auto KpPk = KpSk->producePublicKey();
  if (!KpPk) {
    return WasiCryptoUnexpect(KpPk);
  }

  //  auto Kp = X25519KeyPair{Alg, *Pk, *Sk};
  return KxKeyPair{
      std::make_unique<KeyPair>(Alg, std::move(*KpPk), std::move(*KpSk))};
}

WasiCryptoExpect<void> KeyPair::verify() {}

WasiCryptoExpect<KxPublicKey> KeyPair::publicKey() {
  return KxPublicKey{std::make_unique<PublicKey>(std::move(Pk))};
}

WasiCryptoExpect<KxSecretKey> KeyPair::secretKey() {
  return KxSecretKey{std::make_unique<SecretKey>(std::move(Sk))};
}

} // namespace X25519
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
