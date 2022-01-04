// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/key_exchange/alg.h"
#include "host/wasi_crypto/key_exchange/options.h"
#include "host/wasi_crypto/key_exchange/publickey.h"
#include "host/wasi_crypto/key_exchange/secretkey.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class KxKeyPair {
public:
  class Base {
  public:
    virtual ~Base() = default;

    virtual KxAlgorithm alg() = 0;

    virtual WasiCryptoExpect<void> verify() { return {}; }

    virtual WasiCryptoExpect<KxPublicKey> publicKey() = 0;

    virtual WasiCryptoExpect<KxSecretKey> secretKey() = 0;

    virtual WasiCryptoExpect<std::vector<uint8_t>> asRaw();

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_keypair_encoding_e_t Encoding);
  };

  class Builder {
  public:
    virtual ~Builder() = default;

    virtual WasiCryptoExpect<KxKeyPair>
    generate(std::optional<KxOptions> Options) = 0;

    virtual WasiCryptoExpect<KxKeyPair>
    import(Span<uint8_t const> Raw, __wasi_keypair_encoding_e_t Encoding) = 0;
  };

  KxKeyPair(std::unique_ptr<Base> Inner)
      : Inner(
            std::make_shared<Mutex<std::unique_ptr<Base>>>(std::move(Inner))) {}

  static WasiCryptoExpect<KxKeyPair> generate(KxAlgorithm Alg,
                                              std::optional<KxOptions> Options);

  static WasiCryptoExpect<KxKeyPair>
  import(KxAlgorithm Alg, Span<uint8_t const> Raw,
         __wasi_keypair_encoding_e_t Encoding);

  auto &inner() { return Inner; }

private:
  static WasiCryptoExpect<std::unique_ptr<Builder>> builder(KxAlgorithm Alg);

  std::shared_ptr<Mutex<std::unique_ptr<Base>>> Inner;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
