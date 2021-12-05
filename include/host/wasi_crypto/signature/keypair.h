// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/signature/alg.h"
#include "host/wasi_crypto/signature/ecdsa.h"
#include "host/wasi_crypto/signature/eddsa.h"
#include "host/wasi_crypto/signature/options.h"
#include "host/wasi_crypto/signature/publickey.h"
#include "host/wasi_crypto/signature/rsa.h"
#include "host/wasi_crypto/signature/secretkey.h"
#include "host/wasi_crypto/varianthelper.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class SignatureKeyPair
    : public VariantTemplate<EcdsaSignatureKeyPair, EddsaSignatureKeyPair,
                             RsaSignatureKeyPair> {
public:
  using VariantTemplate<EcdsaSignatureKeyPair, EddsaSignatureKeyPair,
                        RsaSignatureKeyPair>::VariantTemplate;

  static WasiCryptoExpect<SignatureKeyPair>
  generate(SignatureAlgorithm Alg, std::optional<SignatureOptions> Options);

  static WasiCryptoExpect<SignatureKeyPair>
  import(SignatureAlgorithm Alg, Span<uint8_t const> Encoded,
         __wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_keypair_encoding_e_t Encoding);

  WasiCryptoExpect<SignaturePublicKey> publicKey();

  WasiCryptoExpect<SignatureSecretKey> secretKey();
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
