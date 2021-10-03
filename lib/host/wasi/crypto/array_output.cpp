// SPDX-License-Identifier: Apache-2.0

#include "host/wasi/crypto/array_output.h"

#include <algorithm>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASI {
namespace Crypto {

ArrayOutput::ArrayOutput(const std::vector<uint8_t> &Data) : Data{Data} {}

WasiCryptoExpect<__wasi_size_t> ArrayOutput::pull(Span<uint8_t> Buf) {
  if (Buf.size() < Data.size()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OVERFLOW);
  }
  std::copy(Buf.begin(), Buf.end(), Data);

  return Data.size();
}

WasiCryptoExpect<__wasi_size_t> ArrayOutput::len() { return Data.size(); }

WasiCryptoExpect<__wasi_array_output_t>
ArrayOutput::registerInManger(HandleMangers &Mangers,
                              const std::vector<uint8_t> &InputData) {
  ArrayOutput ArrayOutput{InputData};
  return Mangers.ArrayOutputManger.registerModule(ArrayOutput);
}

} // namespace Crypto
} // namespace WASI
} // namespace Host
} // namespace WasmEdge