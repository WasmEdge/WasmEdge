// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/key_exchange/publickey.h"

#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class KxSecretKeyBase {
public:
  virtual KxAlgorithm alg() = 0;

  virtual WasiCryptoExpect<__wasi_size_t> len() = 0;

  virtual WasiCryptoExpect<Span<uint8_t const>> asRaw() = 0;

  virtual WasiCryptoExpect<KxPublicKey> publicKey() = 0;

  virtual WasiCryptoExpect<std::vector<uint8_t>> dh(KxPublicKey &KxPk);

  virtual WasiCryptoExpect<std::vector<uint8_t>>
      decapsulate(Span<uint8_t const>);
};

class KxSecretKey {
public:
  KxSecretKey(std::unique_ptr<KxSecretKeyBase> Key);

  virtual ~KxSecretKey() = default;

  KxAlgorithm alg();

  WasiCryptoExpect<__wasi_size_t> len();

  WasiCryptoExpect<Span<uint8_t const>> asRaw();

  WasiCryptoExpect<std::vector<uint8_t>> dh(KxPublicKey &KxPk);

  WasiCryptoExpect<std::vector<uint8_t>>
  decapsulate(Span<uint8_t const> EncapsulatedSecret);

private:
  std::shared_ptr<Mutex<std::unique_ptr<KxSecretKeyBase>>> Inner;
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
