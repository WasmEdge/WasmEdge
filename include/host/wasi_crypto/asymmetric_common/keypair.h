// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/key_exchange/keypair.h"
#include "host/wasi_crypto/signature/keypair.h"

#include <variant>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
class KeyPair {
public:
  WasiCryptoExpect<SignatureKeyPair> asSignatureKeyPair();

  WasiCryptoExpect<KxKeyPair> asKxKeyPair();

private:
  std::variant<SignatureKeyPair, KxKeyPair> Inner;
};


} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
