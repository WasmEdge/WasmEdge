// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/asymmetric_common/publickey.h"
#include "host/wasi_crypto/asymmetric_common/secretkey.h"
#include "host/wasi_crypto/common/options.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/key_exchange/keypair.h"
#include "host/wasi_crypto/signature/keypair.h"
#include "host/wasi_crypto/varianthelper.h"

#include <variant>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class KeyPair : public VariantTemplate<SignatureKeyPair, KxKeyPair> {
public:
  using VariantTemplate<SignatureKeyPair, KxKeyPair>::VariantTemplate;

  static WasiCryptoExpect<KeyPair> generate(__wasi_algorithm_type_e_t AlgType,
                                            std::string_view AlgStr,
                                            std::optional<Common::Options> OptOptions);

  static WasiCryptoExpect<KeyPair> import(__wasi_algorithm_type_e_t AlgType,
                                          std::string_view AlgStr,
                                          Span<uint8_t const> Encoded,
                                          __wasi_keypair_encoding_e_t Encoding);

  static WasiCryptoExpect<KeyPair> fromPkAndSk(PublicKey Pk, SecretKey Sk);

  WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_keypair_encoding_e_t Kp);

  WasiCryptoExpect<PublicKey> publicKey();

  WasiCryptoExpect<SecretKey> secretKey();

private:
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
