// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/key_exchange/keypair.h"
#include "host/wasi_crypto/key_exchange/publickey.h"
#include "openssl/evp.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Kx {

class X25519PublicKey : public PublicKey {
public:
  X25519PublicKey(EVP_PKEY *Ctx) : Pk(std::move(Ctx)) {}
  // Raw
  static WasiCryptoExpect<std::unique_ptr<X25519PublicKey>>
  import(KxAlgorithm Alg, Span<uint8_t const> Raw,
         __wasi_publickey_encoding_e_t Encoding);

  //  KxAlgorithm alg() override { return Alg; }

  //  WasiCryptoExpect<__wasi_size_t> len() override { return Len; }

  WasiCryptoExpect<std::vector<uint8_t>> exportData() override;

  WasiCryptoExpect<void> verify() override;

  friend class X25519SecretKey;

  inline static __wasi_size_t Len = 32;

private:
  friend class X25519SKCtx;
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Pk;
};

class X25519SecretKey : public SecretKey {
public:
  X25519SecretKey(EVP_PKEY *Ctx) : Sk(Ctx) {}

  // Raw
  static WasiCryptoExpect<std::unique_ptr<X25519SecretKey>>
  import(Span<uint8_t const> Encoded, __wasi_secretkey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_secretkey_encoding_e_t Encoding) override;

  WasiCryptoExpect<std::unique_ptr<PublicKey>> publicKey() override;

  WasiCryptoExpect<std::vector<uint8_t>>
  dh(std::shared_ptr<PublicKey> Pk) override;

  inline static __wasi_size_t Len = 32;

private:
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Sk;
};

class X25519KeyPair : public KeyPair {
public:
  class Builder : public KeyPair::Builder {
  public:
    Builder(KxAlgorithm Alg) : Alg(Alg) {}

    WasiCryptoExpect<std::unique_ptr<KeyPair>>
    generate(std::optional<Options> Options) override;

    WasiCryptoExpect<std::unique_ptr<KeyPair>>
    import(Span<const uint8_t> Raw,
           __wasi_keypair_encoding_e_t Encoding) override {
      return X25519KeyPair::import(Alg, Raw, Encoding);
    }

  private:
    KxAlgorithm Alg;
  };

  X25519KeyPair(EVP_PKEY *Ctx) : Ctx(Ctx) {}

  static WasiCryptoExpect<std::unique_ptr<KeyPair>>
  import(KxAlgorithm Alg, Span<const uint8_t> Raw,
         __wasi_keypair_encoding_e_t Encoding);

  KxAlgorithm alg() override { return Alg; }

  WasiCryptoExpect<void> verify() override;

  WasiCryptoExpect<std::unique_ptr<PublicKey>> publicKey() override;

  WasiCryptoExpect<std::unique_ptr<SecretKey>> secretKey() override;

private:
  KxAlgorithm Alg;
  OpenSSLUniquePtr<EVP_PKEY, EVP_PKEY_free> Ctx;
};

} // namespace Kx
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
