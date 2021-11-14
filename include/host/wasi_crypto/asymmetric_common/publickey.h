// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/key_exchange/publickey.h"
#include "host/wasi_crypto/signature/publickey.h"

#include <variant>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class PublicKey {
public:
  WasiCryptoExpect<SignaturePublicKey> asSignaturePublicKey();

  WasiCryptoExpect<KxPublickey> asKxPublicKey();

  static WasiCryptoExpect<PublicKey>
  import(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
         Span<uint8_t const> Encoded, __wasi_publickey_encoding_e_t Encoding);

private:
  std::variant<SignaturePublicKey, KxPublickey> Inner;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
