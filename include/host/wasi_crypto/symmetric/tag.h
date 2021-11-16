// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/symmetric/alg.h"
#include "host/wasi_crypto/util.h"

#include <algorithm>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class SymmetricTag {
public:
  SymmetricTag(SymmetricAlgorithm Alg, Span<uint8_t> Data)
      : Alg(Alg), Raw(Data.begin(),  Data.end()) {}

  Span<uint8_t> raw() { return Raw; }

  WasiCryptoExpect<void> verify(Span<uint8_t> RawTag);

private:
  SymmetricAlgorithm Alg;
  std::vector<uint8_t> Raw;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
