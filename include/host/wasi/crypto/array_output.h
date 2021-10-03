// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi/crypto/ctx.h"
#include "host/wasi/crypto/error.h"
#include "wasi/crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WASI {
namespace Crypto {

class ArrayOutput {
public:
  ArrayOutput(const std::vector<uint8_t> &Data);

  WasiCryptoExpect<__wasi_size_t> pull(Span<uint8_t> Buf);

  WasiCryptoExpect<__wasi_size_t> len();

  static WasiCryptoExpect<__wasi_array_output_t>
  registerInManger(HandleMangers &Mangers,
                   const std::vector<uint8_t> &InputData);

private:
  std::vector<uint8_t> Data;
};
} // namespace Crypto
} // namespace WASI
} // namespace Host
} // namespace WasmEdge