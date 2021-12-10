// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/lock.h"
#include "host/wasi_crypto/signature/alg.h"
#include "host/wasi_crypto/signature/signature.h"
#include "host/wasi_crypto/varianthelper.h"

#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class SignaturePublicKey {
public:
  class Base {
  public:
    virtual ~Base() = default;

    virtual SignatureAlgorithm alg() = 0;

    virtual WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_publickey_encoding_e_t Pk) = 0;

    virtual WasiCryptoExpect<SignatureVerificationState> asState() = 0;

    WasiCryptoExpect<void> verify() {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    }
  };

  SignaturePublicKey(std::unique_ptr<Base> Inner)
      : Inner(
            std::make_shared<Mutex<std::unique_ptr<Base>>>(std::move(Inner))) {}

  static WasiCryptoExpect<SignaturePublicKey>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_publickey_encoding_e_t Encoding);

  auto &inner() { return Inner; }

private:
  std::shared_ptr<Mutex<std::unique_ptr<Base>>> Inner;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
