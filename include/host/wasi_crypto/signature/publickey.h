// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/signature/alg.h"

#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class SignaturePublicKey {
public:
  static WasiCryptoExpect<SignaturePublicKey>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<void> verify() {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  }
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
