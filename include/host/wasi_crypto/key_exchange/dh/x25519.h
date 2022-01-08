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

    WasiCryptoExpect<KxPublicKey>
    import(Span<uint8_t const> Raw,
           __wasi_publickey_encoding_e_t Encoding) override;

  private:
    KxAlgorithm Alg;
  };

  X25519PublicKey(KxAlgorithm Alg, X25519PKCtx Ctx)
      : Alg(Alg), Ctx(std::move(Ctx)) {}

  // Raw
  static WasiCryptoExpect<X25519PublicKey>
  import(KxAlgorithm Alg, Span<uint8_t const> Raw,
         __wasi_publickey_encoding_e_t Encoding);

  KxAlgorithm alg() override { return Alg; }

  WasiCryptoExpect<__wasi_size_t> len() override { return X25519PKCtx::Len; }

  WasiCryptoExpect<std::vector<uint8_t>> exportData() override;

  WasiCryptoExpect<void> verify() override;

  friend class X25519SecretKey;

private:
  KxAlgorithm Alg;
  X25519PKCtx Ctx;
};

class X25519SecretKey : public KxSecretKey::Base {
public:
  class Builder : public KxSecretKey::Builder {
  public:
    Builder(KxAlgorithm Alg) : Alg(Alg) {}

    WasiCryptoExpect<KxSecretKey>
    import(Span<const uint8_t> Raw,
           __wasi_secretkey_encoding_e_t Encoding) override;

  private:
    KxAlgorithm Alg;
  };

  X25519SecretKey(KxAlgorithm Alg, X25519SKCtx Ctx)
      : Alg(Alg), Ctx(std::move(Ctx)) {}

  static WasiCryptoExpect<X25519SecretKey>
  import(KxAlgorithm Alg, Span<uint8_t const> Raw,
         __wasi_secretkey_encoding_e_t Encoding);

  WasiCryptoExpect<KxPublicKey> publicKey() override;

  KxAlgorithm alg() override { return Alg; }

  WasiCryptoExpect<__wasi_size_t> len() override;

  WasiCryptoExpect<Span<const uint8_t>> exportData() override;

  WasiCryptoExpect<std::vector<uint8_t>>
  dh(std::unique_ptr<KxPublicKey::Base> &KxPk) override;

  WasiCryptoExpect<X25519PublicKey> producePublicKey();

private:
  KxAlgorithm Alg;
  X25519SKCtx Ctx;
};

class X25519KeyPair : public KxKeyPair::Base {
public:
  class Builder : public KxKeyPair::Builder {
  public:
    Builder(KxAlgorithm Alg) : Alg(Alg) {}

    WasiCryptoExpect<KxKeyPair>
    generate(std::optional<KxOptions> Options) override;

    WasiCryptoExpect<KxKeyPair>
    import(Span<const uint8_t> Raw,
           __wasi_keypair_encoding_e_t Encoding) override {
      return X25519KeyPair::import(Alg, Raw, Encoding);
    }

  private:
    KxAlgorithm Alg;
  };

  X25519KeyPair(KxAlgorithm Alg, X25519KpCtx Kp)
      : Alg(Alg), Kp(std::move(Kp)) {}

  static WasiCryptoExpect<KxKeyPair>
  import(KxAlgorithm Alg, Span<const uint8_t> Raw,
         __wasi_keypair_encoding_e_t Encoding);

  KxAlgorithm alg() override { return Alg; }

  WasiCryptoExpect<void> verify() override;

  WasiCryptoExpect<KxPublicKey> publicKey() override;

  WasiCryptoExpect<KxSecretKey> secretKey() override;

private:
  KxAlgorithm Alg;
  X25519KpCtx Kp;
};

class X25519PKCtx {
public:
  X25519PKCtx(OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Ctx)
      : Pk(std::move(Ctx)) {}

  // Raw
  static WasiCryptoExpect<X25519PKCtx>
  import(Span<uint8_t const> Raw, __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>> exportData();

  inline static __wasi_size_t Len = 32;

private:
  friend class X25519SKCtx;
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Pk;
};

class X25519SKCtx {
public:
  X25519SKCtx(OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Ctx)
      : Sk(std::move(Ctx)) {}

  // Raw
  static WasiCryptoExpect<X25519SKCtx>
  import(Span<uint8_t const> Raw, __wasi_secretkey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>> exportData();

  WasiCryptoExpect<X25519PKCtx> producePublicKey();

  WasiCryptoExpect<std::vector<uint8_t>> dh(X25519PKCtx &Pk);

  inline static __wasi_size_t Len = 32;

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Sk;
};

class X25519KpCtx {
public:
  X25519KpCtx(OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Ctx)
      : Ctx(std::move(Ctx)) {}

  WasiCryptoExpect<X25519PKCtx> publicKey();

  WasiCryptoExpect<X25519SKCtx> secretKey();

  static WasiCryptoExpect<X25519KpCtx> generate(KxAlgorithm Alg);

  static WasiCryptoExpect<X25519KpCtx> import();

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Ctx;
};
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
