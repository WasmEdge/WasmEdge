// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/ctx.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoContext::kxDh(__wasi_publickey_t PkHandle,
                        __wasi_secretkey_t SkHandle) {
  auto Pk = PublickeyManger.get(PkHandle);
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  auto KxPk = std::visit(
      Overloaded{
          [](std::shared_ptr<Kx::PublicKey> Kx)
              -> WasiCryptoExpect<std::shared_ptr<Kx::PublicKey>> {
            return Kx;
          },
          [](auto &&) -> WasiCryptoExpect<std::shared_ptr<Kx::PublicKey>> {
            return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
          }},
      *Pk);

  if (!KxPk) {
    return WasiCryptoUnexpect(KxPk);
  }

  auto Sk = SecretkeyManger.get(SkHandle);
  if (!Sk) {
    return WasiCryptoUnexpect(Sk);
  }

  auto KxSk = std::visit(
      Overloaded{
          [](std::shared_ptr<Kx::SecretKey> Kx)
              -> WasiCryptoExpect<std::shared_ptr<Kx::SecretKey>> {
            return Kx;
          },
          [](auto &&) -> WasiCryptoExpect<std::shared_ptr<Kx::SecretKey>> {
            return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
          }},
      *Sk);
  if (!KxPk) {
    return WasiCryptoUnexpect(KxPk);
  }

  auto SharedSecret = (*KxSk)->dh(*KxPk);
  if (!SharedSecret) {
    return WasiCryptoUnexpect(SharedSecret);
  }

  return allocateArrayOutput(std::move(*SharedSecret));
}

WasiCryptoExpect<std::tuple<__wasi_array_output_t, __wasi_array_output_t>>
WasiCryptoContext::kxEncapsulate(__wasi_publickey_t PkHandle) {
  auto Pk = PublickeyManger.get(PkHandle);
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  auto KxPk = std::visit(
      Overloaded{
          [](std::shared_ptr<Kx::PublicKey> Kx)
              -> WasiCryptoExpect<std::shared_ptr<Kx::PublicKey>> {
            return Kx;
          },
          [](auto &&) -> WasiCryptoExpect<std::shared_ptr<Kx::PublicKey>> {
            return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
          }},
      *Pk);
  if (!KxPk) {
    return WasiCryptoUnexpect(KxPk);
  }

  auto EncapsulateSecret = (*KxPk)->encapsulate();
  if (!EncapsulateSecret) {
    return WasiCryptoUnexpect(EncapsulateSecret);
  }

  auto SecretHandle = allocateArrayOutput(std::move(EncapsulateSecret->Secret));
  if (!SecretHandle) {
    return WasiCryptoUnexpect(SecretHandle);
  }

  auto EncapsulatedSecretHandle =
      allocateArrayOutput(std::move(EncapsulateSecret->EncapsulatedSecretData));
  if (!EncapsulatedSecretHandle) {
    return WasiCryptoUnexpect(EncapsulatedSecretHandle);
  }

  return std::make_tuple(*SecretHandle, *EncapsulatedSecretHandle);
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoContext::kxDecapsulate(__wasi_secretkey_t SkHandle,
                                 Span<uint8_t const> EncapsulatedSecret) {
  auto Sk = SecretkeyManger.get(SkHandle);
  if (!Sk) {
    return WasiCryptoUnexpect(Sk);
  }

  auto KxSk = std::visit(
      Overloaded{
          [](std::shared_ptr<Kx::SecretKey> Kx)
              -> WasiCryptoExpect<std::shared_ptr<Kx::SecretKey>> {
            return Kx;
          },
          [](auto &&) -> WasiCryptoExpect<std::shared_ptr<Kx::SecretKey>> {
            return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
          }},
      *Sk);
  if (!KxSk) {
    return WasiCryptoUnexpect(KxSk);
  }

  auto SharedSecret = (*KxSk)->decapsulate(EncapsulatedSecret);
  if (!SharedSecret) {
    return WasiCryptoUnexpect(SharedSecret);
  }

  return allocateArrayOutput(std::move(*SharedSecret));
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
