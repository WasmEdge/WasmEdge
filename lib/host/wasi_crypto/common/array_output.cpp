// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/common/array_output.h"

#include <algorithm>
#include <climits>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Common {

std::tuple<size_t, bool> ArrayOutput::pull(Span<uint8_t> Buf) noexcept {
  size_t OutputSize = std::min(Buf.size(), Data.size() - Pos.load());

  std::copy(
      std::next(Data.begin(),
                static_cast<decltype(Data)::difference_type>(Pos.load())),
      std::next(Data.begin(),
                static_cast<decltype(Data)::difference_type>(Pos + OutputSize)),
      Buf.begin());
  Pos += OutputSize;

  return {OutputSize, Pos.load() + OutputSize == Data.size()};
}

} // namespace Common
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge