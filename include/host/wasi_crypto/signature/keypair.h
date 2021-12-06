// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/signature/alg.h"
#include "host/wasi_crypto/signature/options.h"
#include "host/wasi_crypto/signature/publickey.h"
#include "host/wasi_crypto/signature/secretkey.h"
#include "host/wasi_crypto/varianthelper.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class SignatureKeyPair {
public:
  class Base {
  public:
    virtual WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_keypair_encoding_e_t Encoding) = 0;

    virtual WasiCryptoExpect<SignaturePublicKey> publicKey() = 0;

    virtual WasiCryptoExpect<SignatureSecretKey> secretKey() = 0;
  };

  static WasiCryptoExpect<SignatureKeyPair>
  generate(SignatureAlgorithm Alg, std::optional<SignatureOptions> Options);

  static WasiCryptoExpect<SignatureKeyPair>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_keypair_encoding_e_t Encoding);

  auto &inner() { return Inner; }

private:
  std::shared_ptr<Mutex<std::unique_ptr<Base>>> Inner;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
