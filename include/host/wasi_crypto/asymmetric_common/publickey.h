// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/key_exchange/publickey.h"
#include "host/wasi_crypto/signature/publickey.h"
#include "host/wasi_crypto/varianthelper.h"

#include <variant>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class PublicKey : public VariantTemplate<SignaturePublicKey, KxPublicKey> {
public:
  using VariantTemplate<SignaturePublicKey, KxPublicKey>::VariantTemplate;

  static WasiCryptoExpect<PublicKey>
  import(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
         Span<uint8_t const> Encoded, __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_publickey_encoding_e_t KxPk);

  WasiCryptoExpect<void> verify();
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
