// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/key_exchange/secretkey.h"
#include "host/wasi_crypto/signature/secretkey.h"
#include "host/wasi_crypto/varianthelper.h"
#include "host/wasi_crypto/asymmetric_common/publickey.h"

#include <variant>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class SecretKey : public VariantTemplate<KxSecretKey, SignatureSecretKey> {
public:
  using VariantTemplate<KxSecretKey, SignatureSecretKey>::VariantTemplate;

  static WasiCryptoExpect<SecretKey>
  import(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
         Span<uint8_t const> Encoded, __wasi_secretkey_encoding_e_t Encoding);

  static WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_secretkey_encoding_e_t SkEncoding);

  WasiCryptoExpect<PublicKey>
  publicKey();
private:
  std::variant<SignatureSecretKey, KxSecretKey> Inner;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
