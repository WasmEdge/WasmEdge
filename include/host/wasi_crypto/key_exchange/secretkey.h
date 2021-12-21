// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/key_exchange/options.h"
#include "host/wasi_crypto/key_exchange/publickey.h"

#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class KxSecretKey {
public:
  class Base {
  public:
    virtual ~Base() = default;

    virtual KxAlgorithm alg() = 0;

    virtual WasiCryptoExpect<__wasi_size_t> len() = 0;

    virtual WasiCryptoExpect<Span<uint8_t const>> asRef() = 0;

    virtual WasiCryptoExpect<KxPublicKey> publicKey() = 0;

    virtual WasiCryptoExpect<std::vector<uint8_t>>
    dh(std::unique_ptr<KxPublicKey::Base> &) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
    }

    virtual WasiCryptoExpect<std::vector<uint8_t>>
    decapsulate(Span<uint8_t const>) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
    }

    virtual WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_secretkey_encoding_e_t) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    }
  };

  class Builder {
  public:
    virtual ~Builder() = default;

    virtual WasiCryptoExpect<KxSecretKey>
    import(Span<uint8_t const> Raw, __wasi_secretkey_encoding_e_t Encoding) = 0;
  };

  KxSecretKey(std::unique_ptr<Base> Inner)
      : Inner(
            std::make_shared<Mutex<std::unique_ptr<Base>>>(std::move(Inner))) {}

  static WasiCryptoExpect<KxSecretKey>
  import(KxAlgorithm Alg, Span<const uint8_t> Encoded,
         __wasi_secretkey_encoding_e_t Encoding);

  auto &inner() { return Inner; }

private:
  static WasiCryptoExpect<std::unique_ptr<Builder>> builder(KxAlgorithm Alg);

  std::shared_ptr<Mutex<std::unique_ptr<Base>>> Inner;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
