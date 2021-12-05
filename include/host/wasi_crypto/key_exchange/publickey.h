// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/key_exchange/alg.h"
#include "host/wasi_crypto/lock.h"

#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

struct EncapsulatedSecret {
  std::vector<uint8_t> EncapsulatedSecretData;
  std::vector<uint8_t> Secret;
};

class KxPublicKey;

class KxPublicKeyBuilder {
public:
  virtual ~KxPublicKeyBuilder() = default;

  virtual WasiCryptoExpect<KxPublicKey> fromRaw(Span<uint8_t const> Raw) = 0;
};

class KxPublicKeyBase {
public:
  virtual ~KxPublicKeyBase() = default;

  virtual KxAlgorithm alg() = 0;

  virtual WasiCryptoExpect<__wasi_size_t> len() = 0;

  virtual WasiCryptoExpect<Span<uint8_t const>> asRaw() = 0;

  virtual WasiCryptoExpect<void> verify();

  virtual WasiCryptoExpect<EncapsulatedSecret> encapsulate();
};

class KxPublicKey {
public:
  KxPublicKey(std::unique_ptr<KxPublicKeyBase> Key);

  KxAlgorithm alg();

  WasiCryptoExpect<__wasi_size_t> len();

  WasiCryptoExpect<std::vector<uint8_t>> asRaw();

  WasiCryptoExpect<void> verify();

  WasiCryptoExpect<EncapsulatedSecret> encapsulate();

  std::shared_ptr<Mutex<std::unique_ptr<KxPublicKeyBase>>>& inner();

  WasiCryptoExpect<std::vector<uint8_t>> exportData(__wasi_publickey_encoding_e_t Encoding);

  //  static WasiCryptoExpect<std::unique_ptr<KxPublicKeyBuilder>>
  //  builder(KxAlgorithm Alg);

private:
  std::shared_ptr<Mutex<std::unique_ptr<KxPublicKeyBase>>> Inner;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
