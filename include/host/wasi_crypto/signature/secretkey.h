// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/signature/ecdsa.h"
#include "host/wasi_crypto/signature/eddsa.h"
#include "host/wasi_crypto/signature/rsa.h"
#include "host/wasi_crypto/varianthelper.h"
#include "host/wasi_crypto/signature/publickey.h"

#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class SignatureSecretKey
    : public VariantTemplate<EcdsaSignatureSecretKey, EddsaSignatureSecretKey,
                             RsaSignatureSecretKey> {
public:
  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_secretkey_encoding_e_t) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  }

  WasiCryptoExpect<SignaturePublicKey> publicKey() {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
  }
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
