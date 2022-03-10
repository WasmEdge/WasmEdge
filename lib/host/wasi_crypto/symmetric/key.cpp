// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/utils/error.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

std::vector<uint8_t> keyExportData(const KeyVariant &KeyVariant) noexcept {
  return std::visit([](auto &&Key) noexcept { return Key.exportData(); },
                    KeyVariant);
}

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge