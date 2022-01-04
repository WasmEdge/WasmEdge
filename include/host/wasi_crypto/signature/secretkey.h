// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/signature/publickey.h"
#include "host/wasi_crypto/varianthelper.h"

#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class SignatureSecretKey {
public:
  class Base {
  public:
    virtual ~Base() = default;

    virtual WasiCryptoExpect<std::vector<uint8_t>>
        exportData(__wasi_secretkey_encoding_e_t) = 0;

    WasiCryptoExpect<SignaturePublicKey> publicKey() {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    }
  };

  SignatureSecretKey(std::unique_ptr<Base> Inner)
      : Inner(
            std::make_shared<Mutex<std::unique_ptr<Base>>>(std::move(Inner))) {}

  static WasiCryptoExpect<SignatureSecretKey>
  import(SignatureAlgorithm, Span<const uint8_t>,
         __wasi_secretkey_encoding_e_t) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  }

  auto &inner() { return Inner; }

private:
  std::shared_ptr<Mutex<std::unique_ptr<Base>>> Inner;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
