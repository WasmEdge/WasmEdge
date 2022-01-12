// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "wasi_crypto/api.hpp"

#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Common {
class ArrayOutput {
public:
  ArrayOutput(std::vector<uint8_t> &&Data) : Data(std::move(Data)) {}

  WasiCryptoExpect<__wasi_size_t> pull(Span<uint8_t> Buf);

  WasiCryptoExpect<__wasi_size_t> len();

private:
  std::vector<uint8_t> Data;
};

} // namespace Common

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge