// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/signatures/signatures.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

WasiCryptoExpect<std::vector<uint8_t>>
sigExportData(const SigVariant &SigVariant,
              __wasi_signature_encoding_e_t Encoding) noexcept {
  return std::visit(
      [Encoding](auto &Sig) noexcept { return Sig.exportData(Encoding); },
      SigVariant);
}

} // namespace Signatures
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
