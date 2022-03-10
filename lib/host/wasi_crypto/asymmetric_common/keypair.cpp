// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/asymmetric_common/keypair.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace AsymmetricCommon {

WasiCryptoExpect<std::vector<uint8_t>>
kpExportData(const KpVariant &KpVariant,
             __wasi_keypair_encoding_e_t Encoding) noexcept {
  return std::visit([=](auto &&Kp) noexcept { return Kp.exportData(Encoding); },
                    KpVariant);
}

WasiCryptoExpect<PkVariant> kpPublicKey(const KpVariant &KpVariant) noexcept {
  return std::visit(
      [=](auto &&Kp) noexcept {
        return Kp.publicKey().map(
            [](auto &&Pk) noexcept { return PkVariant{Pk}; });
      },
      KpVariant);
}

WasiCryptoExpect<SkVariant> kpSecretKey(const KpVariant &KpVariant) noexcept {
  return std::visit(
      [=](auto &&Kp) noexcept {
        return Kp.secretKey().map(
            [](auto &&Sk) noexcept { return SkVariant{Sk}; });
      },
      KpVariant);
}

} // namespace AsymmetricCommon
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge