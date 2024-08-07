// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "signatures/signatures.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

WasiCryptoExpect<SigVariant>
sigImport(Algorithm Alg, Span<const uint8_t> Encoded,
          __wasi_signature_encoding_e_t Encoding) noexcept {
  return std::visit(
      [=](auto Factory) noexcept {
        return decltype(Factory)::Signature::import(Encoded, Encoding)
            .map([](auto &&Sig) noexcept {
              return SigVariant{std::forward<decltype(Sig)>(Sig)};
            });
      },
      Alg);
}

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
