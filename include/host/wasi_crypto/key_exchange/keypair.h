// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/key_exchange/alg.h"
#include "host/wasi_crypto/key_exchange/options.h"
#include "host/wasi_crypto/key_exchange/publickey.h"
#include "host/wasi_crypto/key_exchange/secretkey.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Kx {

class KeyPair {
public:
  virtual ~KeyPair() = default;

  virtual KxAlgorithm alg() = 0;

  virtual WasiCryptoExpect<void> verify() { return {}; }

  virtual WasiCryptoExpect<std::unique_ptr<PublicKey>> publicKey() = 0;

  virtual WasiCryptoExpect<std::unique_ptr<SecretKey>> secretKey() = 0;

  virtual WasiCryptoExpect<std::vector<uint8_t>> asRaw();

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_keypair_encoding_e_t Encoding);

  class Builder {
  public:
    virtual ~Builder() = default;

    virtual WasiCryptoExpect<KeyPair>
    generate(std::optional<Options> Options) = 0;

    virtual WasiCryptoExpect<KeyPair>
    import(Span<uint8_t const> Raw, __wasi_keypair_encoding_e_t Encoding) = 0;

    static WasiCryptoExpect<std::unique_ptr<Builder>> builder(KxAlgorithm Alg);
  };

  static WasiCryptoExpect<std::unique_ptr<KeyPair>>
  generate(KxAlgorithm Alg, std::optional<Options> Options);

  static WasiCryptoExpect<KeyPair> import(KxAlgorithm Alg,
                                          Span<uint8_t const> Raw,
                                          __wasi_keypair_encoding_e_t Encoding);
};

} // namespace Kx
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
