// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/key_exchange/keypair.h"
#include "host/wasi_crypto/key_exchange/publickey.h"

#include "host/wasi_crypto/evpwrapper.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Kx {

class X25519PublicKey : public PublicKey {
public:
  X25519PublicKey(EvpPkeyPtr Ctx) : Ctx(std::move(Ctx)) {}
  // Raw
  static WasiCryptoExpect<std::unique_ptr<X25519PublicKey>>
  import(Span<uint8_t const> Encoded, __wasi_publickey_encoding_e_t Encoding);

  //  KxAlgorithm alg() override { return Alg; }

  //  WasiCryptoExpect<size_t> len() override { return Len; }

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_publickey_encoding_e_t Encoding) override;

  WasiCryptoExpect<void> verify() override;

  friend class X25519SecretKey;

  inline static size_t PkSize = 32;

private:
  EvpPkeyPtr Ctx;
};

class X25519SecretKey : public SecretKey {
public:
  X25519SecretKey(EvpPkeyPtr Ctx) : Ctx(std::move(Ctx)) {}

  // Raw
  static WasiCryptoExpect<std::unique_ptr<X25519SecretKey>>
  import(Span<uint8_t const> Encoded, __wasi_secretkey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_secretkey_encoding_e_t Encoding) override;

  WasiCryptoExpect<std::unique_ptr<PublicKey>> publicKey() override;

  WasiCryptoExpect<std::vector<uint8_t>>
  dh(std::shared_ptr<PublicKey> Pk) override;

  inline static size_t SkSize = 32;

private:
  EvpPkeyPtr Ctx;
};

class X25519KeyPair : public KeyPair {
public:
  class Builder : public KeyPair::Builder {
  public:
    WasiCryptoExpect<std::unique_ptr<KeyPair>>
    generate(std::shared_ptr<Options> Options) override;

    WasiCryptoExpect<std::unique_ptr<KeyPair>>
    import(Span<const uint8_t> Raw,
           __wasi_keypair_encoding_e_t Encoding) override;
  };

  X25519KeyPair(EvpPkeyPtr Ctx) : Ctx(std::move(Ctx)) {}

  WasiCryptoExpect<void> verify() override;

  WasiCryptoExpect<std::unique_ptr<PublicKey>> publicKey() override;

  WasiCryptoExpect<std::unique_ptr<SecretKey>> secretKey() override;

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_keypair_encoding_e_t Encoding) override;

private:
  EvpPkeyPtr Ctx;
};

} // namespace Kx
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
