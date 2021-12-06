// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/key_exchange/keypair.h"
#include "host/wasi_crypto/key_exchange/publickey.h"
#include "host/wasi_crypto/wrapper/x25519.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class X25519PublicKeyBuilder : public KxPublicKeyBuilder {
public:
  WasiCryptoExpect<KxPublicKey> fromRaw(Span<const uint8_t> Raw) override;

private:
  KxAlgorithm Alg;
};

class X25519PublicKey : public KxPublicKey::Base {
public:
  static WasiCryptoExpect<X25519PublicKey> make(KxAlgorithm Alg,
                                                Span<uint8_t const> Raw);

  KxAlgorithm alg() override;

  WasiCryptoExpect<__wasi_size_t> len() override;

  WasiCryptoExpect<Span<const uint8_t>> asRaw() override;

  WasiCryptoExpect<void> verify() override;

  X25519PublicKey(KxAlgorithm Alg, X25519PK Ctx);

private:
  KxAlgorithm Alg;
  X25519PK Ctx;
};

class X25519SecretKeyBuilder : public KxSecretKeyBuilder {
public:
  WasiCryptoExpect<KxSecretKey> fromRaw(Span<const uint8_t> Raw) override;

private:
  KxAlgorithm Alg;
};

class X25519SecretKey : public KxSecretKey::Base {
public:
  static WasiCryptoExpect<X25519SecretKey> make(KxAlgorithm Alg,
                                                Span<uint8_t const> Raw);

  WasiCryptoExpect<X25519PublicKey> producePublicKey();

  WasiCryptoExpect<KxPublicKey> publicKey() override;

  KxAlgorithm alg() override;

  WasiCryptoExpect<__wasi_size_t> len() override;

  WasiCryptoExpect<Span<const uint8_t>> asRaw() override;

  WasiCryptoExpect<std::vector<uint8_t>> dh(KxPublicKey &KxPk) override;

  X25519SecretKey(KxAlgorithm Alg, X25519SK Ctx);

private:
  KxAlgorithm Alg;
  X25519SK Ctx;
};

class X25519KeyPairBuilder : public KxKeyPairBuilder {
public:
  X25519KeyPairBuilder(KxAlgorithm Alg);

  WasiCryptoExpect<KxKeyPair>
  generate(std::optional<KxOptions> Options) override;

private:
  KxAlgorithm Alg;
};

class X25519KeyPair : public KxKeyPairBase {
public:
  X25519KeyPair(KxAlgorithm Alg, X25519PublicKey PublicKey,
                X25519SecretKey SecretKey);

  KxAlgorithm alg() override;

  WasiCryptoExpect<void> verify() override;

  WasiCryptoExpect<KxPublicKey> publicKey() override;

  WasiCryptoExpect<KxSecretKey> secretKey() override;

private:
  KxAlgorithm Alg;
  X25519PublicKey PublicKey;
  X25519SecretKey SecretKey;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
