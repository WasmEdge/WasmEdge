// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ctx.h"
#include "kx/kx.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

WasiCryptoExpect<__wasi_array_output_t>
Context::kxDh(__wasi_kx_publickey_t PkHandle,
              __wasi_kx_secretkey_t SkHandle) noexcept {
  auto Sk = SecretKeyManager.getAs<Kx::SkVariant>(SkHandle);
  if (!Sk) {
    return WasiCryptoUnexpect(Sk);
  }

  auto Pk = PublicKeyManager.getAs<Kx::PkVariant>(PkHandle);
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  return Kx::dh(*Pk, *Sk).and_then([this](auto &&Data) {
    return ArrayOutputManager.registerManager(
        std::forward<decltype(Data)>(Data));
  });
}

WasiCryptoExpect<std::tuple<__wasi_array_output_t, __wasi_array_output_t>>
Context::kxEncapsulate(__wasi_kx_publickey_t PkHandle) noexcept {
  auto EncapsulatedSecret =
      PublicKeyManager.getAs<Kx::PkVariant>(PkHandle).and_then(
          [](auto &&KxPk) noexcept { return Kx::encapsulate(KxPk); });
  if (!EncapsulatedSecret) {
    return WasiCryptoUnexpect(EncapsulatedSecret);
  }

  auto SecretHandle =
      ArrayOutputManager.registerManager(std::move(EncapsulatedSecret->Secret));
  if (!SecretHandle) {
    return WasiCryptoUnexpect(SecretHandle);
  }

  auto EncapsulatedSecretHandle = ArrayOutputManager.registerManager(
      std::move(EncapsulatedSecret->EncapsulatedSecretData));
  if (!EncapsulatedSecretHandle) {
    return WasiCryptoUnexpect(EncapsulatedSecretHandle);
  }

  return std::tuple(*SecretHandle, *EncapsulatedSecretHandle);
}

WasiCryptoExpect<__wasi_array_output_t>
Context::kxDecapsulate(__wasi_kx_secretkey_t SkHandle,
                       Span<const uint8_t> EncapsulatedSecret) noexcept {
  return SecretKeyManager.getAs<Kx::SkVariant>(SkHandle)
      .and_then([EncapsulatedSecret](auto &&KxSk) noexcept {
        return Kx::decapsulate(KxSk, EncapsulatedSecret);
      })
      .and_then([this](auto &&Secret) noexcept {
        return ArrayOutputManager.registerManager(
            std::forward<decltype(Secret)>(Secret));
      });
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
