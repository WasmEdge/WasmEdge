// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/signature/alg.h"
#include "host/wasi_crypto/signature/ecdsa.h"
#include "host/wasi_crypto/signature/eddsa.h"
#include "host/wasi_crypto/signature/rsa.h"
#include "host/wasi_crypto/varianthelper.h"

#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class SignaturePublicKey
    : public VariantTemplate<EcdsaSignaturePublicKey, EddsaSignaturePublicKey,
                             RsaSignaturePublicKey> {
public:
  using VariantTemplate<EcdsaSignaturePublicKey, EddsaSignaturePublicKey,
                        RsaSignaturePublicKey>::VariantTemplate;

  static WasiCryptoExpect<SignaturePublicKey>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_publickey_encoding_e_t Pk);

  SignatureAlgorithm alg();

  WasiCryptoExpect<void> verify();
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
