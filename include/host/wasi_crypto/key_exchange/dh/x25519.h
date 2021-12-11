// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/key_exchange/keypair.h"
#include "host/wasi_crypto/key_exchange/publickey.h"
#include "host/wasi_crypto/wrapper/x25519.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class X25519PublicKey : public KxPublicKey::Base {
public:
  class Builder : public KxPublicKey::Builder {
  public:
    Builder(KxAlgorithm Alg) : Alg(Alg) {}

    WasiCryptoExpect<KxPublicKey> fromRaw(Span<const uint8_t> Raw) override;

  private:
    KxAlgorithm Alg;
  };

  X25519PublicKey(KxAlgorithm Alg, X25519PK Ctx)
      : Alg(Alg), Ctx(std::move(Ctx)) {}

  static WasiCryptoExpect<X25519PublicKey> import(KxAlgorithm Alg,
                                                  Span<uint8_t const> Raw);

  KxAlgorithm alg() override { return Alg; }

  WasiCryptoExpect<__wasi_size_t> len() override { return X25519PK::Len; }

  WasiCryptoExpect<std::vector<uint8_t>> asRaw() override;

  WasiCryptoExpect<void> verify() override;

  friend class X25519SecretKey;

private:
  KxAlgorithm Alg;
  X25519PK Ctx;
};

class X25519SecretKey : public KxSecretKey::Base {
public:
  class Builder : public KxSecretKey::Builder {
  public:
    Builder(KxAlgorithm Alg) : Alg(Alg) {}

    WasiCryptoExpect<KxSecretKey> fromRaw(Span<const uint8_t> Raw) override;

  private:
    KxAlgorithm Alg;
  };

  X25519SecretKey(KxAlgorithm Alg, X25519SK Ctx)
      : Alg(Alg), Ctx(std::move(Ctx)) {}

  static WasiCryptoExpect<X25519SecretKey> import(KxAlgorithm Alg,
                                                  Span<uint8_t const> Raw);

  WasiCryptoExpect<KxPublicKey> publicKey() override;

  KxAlgorithm alg() override { return Alg; }

  WasiCryptoExpect<__wasi_size_t> len() override;

  WasiCryptoExpect<Span<const uint8_t>> asRaw() override;

  WasiCryptoExpect<std::vector<uint8_t>>
  dh(std::unique_ptr<KxPublicKey::Base> &KxPk) override;

  WasiCryptoExpect<X25519PublicKey> producePublicKey();

private:
  KxAlgorithm Alg;
  X25519SK Ctx;
};

class X25519KeyPair : public KxKeyPair::Base {
public:
  class Builder : public KxKeyPair::Builder {
  public:
    Builder(KxAlgorithm Alg) : Alg(Alg) {}

    WasiCryptoExpect<KxKeyPair>
    generate(std::optional<KxOptions> Options) override;

  private:
    KxAlgorithm Alg;
  };

  X25519KeyPair(KxAlgorithm Alg, X25519PublicKey Pk, X25519SecretKey Sk)
      : Alg(Alg), Pk(std::move(Pk)), Sk(std::move(Sk)) {}

  KxAlgorithm alg() override { return Alg; }

  WasiCryptoExpect<void> verify() override;

  WasiCryptoExpect<KxPublicKey> publicKey() override;

  WasiCryptoExpect<KxSecretKey> secretKey() override;

private:
  KxAlgorithm Alg;
  X25519PublicKey Pk;
  X25519SecretKey Sk;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
