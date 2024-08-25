// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "asymmetric_common/secretkey.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace AsymmetricCommon {

WasiCryptoExpect<SkVariant>
importSk(AsymmetricCommon::Algorithm Alg, Span<const uint8_t> Encoded,
         __wasi_secretkey_encoding_e_t Encoding) noexcept {
  return std::visit(
      [=](auto Factory) noexcept -> WasiCryptoExpect<SkVariant> {
        return decltype(Factory)::SecretKey::import(Encoded, Encoding);
      },
      Alg);
}

WasiCryptoExpect<SecretVec>
skExportData(const SkVariant &SkVariant,
             __wasi_secretkey_encoding_e_t Encoding) noexcept {
  return std::visit(
      [Encoding](const auto &Sk) noexcept { return Sk.exportData(Encoding); },
      SkVariant);
}

WasiCryptoExpect<PkVariant> skPublicKey(const SkVariant &SkVariant) noexcept {
  return std::visit(
      [](const auto &Sk) noexcept {
        return Sk.publicKey().map([](auto &&Pk) noexcept {
          return PkVariant{std::forward<decltype(Pk)>(Pk)};
        });
      },
      SkVariant);
}

} // namespace AsymmetricCommon
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
