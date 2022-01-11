// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/key_exchange/alg.h"
#include "host/wasi_crypto/key_exchange/options.h"
#include "host/wasi_crypto/lock.h"

#include <memory>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Kx {

struct EncapsulatedSecret {
  std::vector<uint8_t> EncapsulatedSecretData;
  std::vector<uint8_t> Secret;
};

class PublicKey {
public:
  virtual ~PublicKey() = default;

  virtual WasiCryptoExpect<void> verify() {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  }

  virtual WasiCryptoExpect<EncapsulatedSecret> encapsulate() {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }

  virtual WasiCryptoExpect<std::vector<uint8_t>>
      exportData(__wasi_publickey_encoding_e_t) = 0;

  static WasiCryptoExpect<std::unique_ptr<PublicKey>>
  import(KxAlgorithm Alg, Span<uint8_t const> Raw,
         __wasi_publickey_encoding_e_t Encoding);
};

} // namespace Kx
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
