// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/common/array_output.h"

#include <algorithm>
#include <climits>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Common {

std::tuple<size_t, bool> ArrayOutput::pull(Span<uint8_t> Buf) {
  size_t CurrentPos = Pos.load();
  auto OutputSize = std::min(Buf.size(), Data.size() - CurrentPos);
  std::copy(Data.begin() + CurrentPos, Data.begin() + CurrentPos + OutputSize,
            Buf.begin());

  Pos.fetch_add(OutputSize);

  return {OutputSize, CurrentPos + OutputSize == Data.size()};
}

size_t ArrayOutput::len() { return Data.size(); }

} // namespace Common
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge