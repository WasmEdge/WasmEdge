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
  Signature(SignatureAlgorithm Alg, std::vector<uint8_t> &&Data)
      : Alg(Alg), Data(std::move(Data)) {}

  auto alg() { return Alg; }

  const auto &data() const { return Data; }

  virtual ~Signature() = default;

  static WasiCryptoExpect<std::unique_ptr<Signature>>
  import(SignatureAlgorithm Alg, Span<const uint8_t> Encoded,
         __wasi_signature_encoding_e_t Encoding);

  virtual WasiCryptoExpect<std::vector<uint8_t>>
  exportData(__wasi_signature_encoding_e_t Encoding) = 0;

protected:
  const SignatureAlgorithm Alg;
  const std::vector<uint8_t> Data;
};

} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
