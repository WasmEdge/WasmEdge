// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/lock.h"
#include "host/wasi_crypto/signature/alg.h"

#include "host/wasi_crypto/signature/alg.h"
#include <memory>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

class Signature {
public:
  virtual ~Signature() = default;

  static WasiCryptoExpect<std::unique_ptr<Signature>>
  import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
         __wasi_signature_encoding_e_t Encoding);

  virtual WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_signature_encoding_e_t Encoding) = 0;
};

} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
