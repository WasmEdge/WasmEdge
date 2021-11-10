// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/common/array_output.h"

#include <algorithm>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

ArrayOutput::ArrayOutput(Span<uint8_t> Data) : Data(Data.begin(),  Data.end()) {}

WasiCryptoExpect<__wasi_size_t> ArrayOutput::pull(Span<uint8_t> Buf) {
  if (Buf.size() < Data.size()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OVERFLOW);
  }

  std::copy(Data.begin(), Data.end(), Buf.begin());

  return Data.size();
}

WasiCryptoExpect<__wasi_size_t> ArrayOutput::len() { return Data.size(); }

// WasiCryptoExpect<__wasi_array_output_t>
// ArrayOutput::registerInManger(HandleMangers &Mangers,
//                               const std::vector<uint8_t> &InputData) {
//   ArrayOutput ArrayOutput{InputData};
//   return Mangers.ArrayOutputManger.registerManger(ArrayOutput);
// }

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge