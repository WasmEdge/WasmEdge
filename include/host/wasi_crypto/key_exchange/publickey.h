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

class KxPublicKey {
public:
  class Base {
  public:
    virtual ~Base() = default;

    virtual KxAlgorithm alg() = 0;

    virtual WasiCryptoExpect<__wasi_size_t> len() = 0;

    virtual WasiCryptoExpect<std::vector<uint8_t>> asRaw() = 0;

    virtual WasiCryptoExpect<void> verify();

    virtual WasiCryptoExpect<EncapsulatedSecret> encapsulate() {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
    }

    virtual WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_publickey_encoding_e_t) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    }
  };

  class Builder {
  public:
    virtual ~Builder() = default;

    virtual WasiCryptoExpect<KxPublicKey> fromRaw(Span<uint8_t const> Raw) = 0;
  };

  KxPublicKey(std::unique_ptr<Base> Inner)
      : Inner(
            std::make_shared<Mutex<std::unique_ptr<Base>>>(std::move(Inner))) {}

  auto &inner() { return Inner; }

  //  static WasiCryptoExpect<std::unique_ptr<KxPublicKeyBuilder>>
  //  builder(KxAlgorithm Alg);

private:
  std::shared_ptr<Mutex<std::unique_ptr<Base>>> Inner;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
