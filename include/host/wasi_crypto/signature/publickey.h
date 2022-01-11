// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/signature/alg.h"
#include "host/wasi_crypto/signature/state.h"

#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

class PublicKey {
public:
  virtual ~PublicKey() = default;

  virtual WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_publickey_encoding_e_t Pk) = 0;

  virtual WasiCryptoExpect<std::unique_ptr<VerificationState>>
  openVerificationState() = 0;

  WasiCryptoExpect<void> verify() {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  }

  static WasiCryptoExpect<std::unique_ptr<PublicKey>>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_publickey_encoding_e_t Encoding);
};

} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
