// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/key_exchange/alg.h"
#include "host/wasi_crypto/key_exchange/options.h"
#include "host/wasi_crypto/key_exchange/publickey.h"
#include "host/wasi_crypto/key_exchange/secretkey.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
class KxKeyPair;

class KxKeyPairBuilder {
public:
  virtual WasiCryptoExpect<KxKeyPair>
  generate(std::optional<KxOptions> Options) = 0;
};

class KxKeyPairBase {
public:
  virtual ~KxKeyPairBase() = default;

  virtual KxAlgorithm alg() = 0;

  virtual WasiCryptoExpect<void> verify() { return {}; }

  virtual WasiCryptoExpect<KxPublicKey> publicKey() = 0;

  virtual WasiCryptoExpect<KxSecretKey> secretKey() = 0;

  virtual WasiCryptoExpect<std::vector<uint8_t>> asRaw() {
    auto Pk = publicKey();
    if (!Pk) {
      return WasiCryptoUnexpect(Pk);
    }
    auto PkRaw = Pk->asRaw();
    if (!PkRaw) {
      return WasiCryptoUnexpect(PkRaw);
    }

    auto Sk = secretKey();
    if (!Sk) {
      return WasiCryptoUnexpect(Sk);
    }
    auto SkRaw = Sk->asRaw();
    if (!SkRaw) {
      return WasiCryptoUnexpect(SkRaw);
    }
    PkRaw->insert(PkRaw->end(), SkRaw->begin(), SkRaw->end());
    return PkRaw;
  }
};

class KxKeyPair {
public:
  KxKeyPair(std::unique_ptr<KxKeyPairBase> Inner);

  static WasiCryptoExpect<std::unique_ptr<KxKeyPairBuilder>>
  builder(KxAlgorithm Alg);

  static WasiCryptoExpect<KxKeyPair> generate(KxAlgorithm Alg,
                                              std::optional<KxOptions> Options);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<KxPublicKey> publicKey() {
    return Inner->locked(
        [](std::unique_ptr<KxKeyPairBase> &Key) { return Key->publicKey(); });
  }

  WasiCryptoExpect<KxSecretKey> secretKey() {
    return Inner->locked(
        [](std::unique_ptr<KxKeyPairBase> &Key) { return Key->secretKey(); });
  }

private:
  std::shared_ptr<Mutex<std::unique_ptr<KxKeyPairBase>>> Inner;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
