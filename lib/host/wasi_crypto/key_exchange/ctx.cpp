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

  auto KxPk = Pk->as<KxPublicKey>();
  if (!KxPk) {
    return WasiCryptoUnexpect(KxPk);
  }

  auto Sk = SecretkeyManger.get(SkHandle);
  if (!Sk) {
    return WasiCryptoUnexpect(Sk);
  }

  auto KxSk = Sk->as<KxSecretKey>();
  if (!KxPk) {
    return WasiCryptoUnexpect(KxPk);
  }

  auto &M1 = *(KxPk->inner());
  auto &M2 = *(KxSk->inner());
  auto SharedSecret = acquireLocked<std::unique_ptr<KxPublicKey::Base>,
                                    std::unique_ptr<KxSecretKey::Base>>(
      M1, M2,
      [](std::unique_ptr<KxPublicKey::Base> &Inner1,
         std::unique_ptr<KxSecretKey::Base> &Inner2) {
        return Inner2->dh(Inner1);
      });

  //  auto SharedSecret =
  //      KxSk->inner()->locked([&KxPk](auto &Inner) { return Inner->dh(*KxPk);
  //      });
  //  if (!SharedSecret) {
  //    return WasiCryptoUnexpect(SharedSecret);
  //  }

  return allocateArrayOutput(std::move(*SharedSecret));
}

WasiCryptoExpect<std::tuple<__wasi_array_output_t, __wasi_array_output_t>>
WasiCryptoContext::kxEncapsulate(__wasi_publickey_t PkHandle) {
  auto Pk = PublickeyManger.get(PkHandle);
  if (!Pk) {
    return WasiCryptoUnexpect(Pk);
  }

  auto KxPk = Pk->as<KxPublicKey>();
  if (!KxPk) {
    return WasiCryptoUnexpect(KxPk);
  }

  auto EncapsulateSecret =
      KxPk->inner()->locked([](auto &Inner) { return Inner->encapsulate(); });
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

  auto KxSk = Sk->as<KxSecretKey>();
  if (!KxSk) {
    return WasiCryptoUnexpect(KxSk);
  }

  auto SharedSecret = KxSk->inner()->locked([&EncapsulatedSecret](auto &Base) {
    return Base->decapsulate(EncapsulatedSecret);
  });
  if (!SharedSecret) {
    return WasiCryptoUnexpect(SharedSecret);
  }

  return allocateArrayOutput(std::move(*SharedSecret));
}

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
