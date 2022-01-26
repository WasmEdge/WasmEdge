// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"
#include "wasi_crypto/api.hpp"

#include <atomic>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Common {
class ArrayOutput {
public:
  ArrayOutput(std::vector<uint8_t> &&Data) : Data(std::move(Data)) {}

  std::tuple<size_t, bool> pull(Span<uint8_t> Buf);

  size_t len();

private:
  const std::vector<uint8_t> Data;
  std::atomic<size_t> Pos = 0;
};

} // namespace Common

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge