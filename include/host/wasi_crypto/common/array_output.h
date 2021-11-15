// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class ArrayOutput {
public:
  ArrayOutput(std::vector<uint8_t>&& Data);

  WasiCryptoExpect<__wasi_size_t> pull(Span<uint8_t> Buf);

  WasiCryptoExpect<__wasi_size_t> len();

private:
  std::vector<uint8_t> Data;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge