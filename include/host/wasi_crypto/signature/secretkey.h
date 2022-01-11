// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/signature/publickey.h"

#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

class SecretKey {
public:
  virtual ~SecretKey() = default;

  virtual WasiCryptoExpect<std::vector<uint8_t>>
      exportData(__wasi_secretkey_encoding_e_t) = 0;

  WasiCryptoExpect<std::unique_ptr<PublicKey>> publicKey() {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  }

  static WasiCryptoExpect<std::unique_ptr<SecretKey>>
      import(SignatureAlgorithm, Span<const uint8_t>,
             __wasi_secretkey_encoding_e_t);
};
} // namespace Signatures

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
