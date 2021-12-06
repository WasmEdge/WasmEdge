// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/key_exchange/publickey.h"

#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class KxSecretKey {
public:
  class Base {
  public:
    virtual KxAlgorithm alg() = 0;

    virtual WasiCryptoExpect<__wasi_size_t> len() = 0;

    virtual WasiCryptoExpect<Span<uint8_t const>> asRaw() = 0;

    virtual WasiCryptoExpect<KxPublicKey> publicKey() = 0;

    virtual WasiCryptoExpect<std::vector<uint8_t>> dh(KxPublicKey &KxPk);

    virtual WasiCryptoExpect<std::vector<uint8_t>>
        decapsulate(Span<uint8_t const>);

    WasiCryptoExpect<std::vector<uint8_t>>
        exportData(__wasi_secretkey_encoding_e_t);

  };

  KxSecretKey(std::unique_ptr<Base> Key);

  virtual ~KxSecretKey() = default;

  auto &inner() { return Inner; }

  WasiCryptoExpect<std::vector<uint8_t>>
      exportData(__wasi_secretkey_encoding_e_t);

  WasiCryptoExpect<KxPublicKey> publicKey();

private:
  std::shared_ptr<Mutex<std::unique_ptr<Base>>> Inner;
};

class KxSecretKeyBuilder {
public:
  virtual ~KxSecretKeyBuilder() = default;

  virtual WasiCryptoExpect<KxSecretKey> fromRaw(Span<uint8_t const> Raw) = 0;

  //  static WasiCryptoExpect<std::unique_ptr<KxSecretKeyBuilder>>
  //  builder(std::string_view Alg);
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
