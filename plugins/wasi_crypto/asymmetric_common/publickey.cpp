// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "asymmetric_common/publickey.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace AsymmetricCommon {

WasiCryptoExpect<PkVariant>
importPk(AsymmetricCommon::Algorithm Alg, Span<const uint8_t> Encoded,
         __wasi_publickey_encoding_e_t Encoding) noexcept {
  return std::visit(
      [=](auto Factory) noexcept -> WasiCryptoExpect<PkVariant> {
        return decltype(Factory)::PublicKey::import(Encoded, Encoding);
      },
      Alg);
}

WasiCryptoExpect<std::vector<uint8_t>>
pkExportData(const PkVariant &PkVariant,
             __wasi_publickey_encoding_e_t Encoding) noexcept {
  return std::visit(
      [Encoding](const auto &Pk) noexcept { return Pk.exportData(Encoding); },
      PkVariant);
}

WasiCryptoExpect<void> pkVerify(const PkVariant &PkVariant) noexcept {
  return std::visit([](const auto &Pk) noexcept { return Pk.verify(); },
                    PkVariant);
}

} // namespace AsymmetricCommon
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
