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
namespace Kx {

class SecretKey {
public:
  virtual ~SecretKey() = default;

  virtual WasiCryptoExpect<std::unique_ptr<PublicKey>> publicKey() = 0;

  virtual WasiCryptoExpect<std::vector<uint8_t>>
      exportData(__wasi_secretkey_encoding_e_t) = 0;

  virtual WasiCryptoExpect<std::vector<uint8_t>>
  dh(std::shared_ptr<PublicKey>) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  virtual WasiCryptoExpect<std::vector<uint8_t>>
  decapsulate(Span<uint8_t const>) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  static WasiCryptoExpect<std::unique_ptr<SecretKey>>
  import(KxAlgorithm Alg, Span<const uint8_t> Encoded,
         __wasi_secretkey_encoding_e_t Encoding);
};

} // namespace Kx

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
