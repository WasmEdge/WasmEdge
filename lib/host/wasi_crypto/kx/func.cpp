// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/kx/func.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

Expect<uint32_t> Dh::body(Runtime::Instance::MemoryInstance *MemInst,
                          int32_t PkHandle, int32_t SkHandle,
                          uint32_t /* Out */ SharedSecretPtr) {
  checkExist(MemInst);

  auto *const SharedSecret =
      MemInst->getPointer<__wasi_array_output_t *>(SharedSecretPtr);
  checkExist(SharedSecret);

  if (auto Res = Ctx.kxDh(PkHandle, SkHandle); unlikely(!Res)) {
    return Res.error();
  } else {
    *SharedSecret = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> Encapsulate::body(Runtime::Instance::MemoryInstance *MemInst,
                                   int32_t PkHandle,
                                   uint32_t /* Out */ SecretPtr,
                                   uint32_t /* Out */ EncapsulatedSecretPtr) {
  checkExist(MemInst);

  auto *const Secret = MemInst->getPointer<__wasi_array_output_t *>(SecretPtr);
  checkExist(Secret);

  auto *const EncapsulatedSecret =
      MemInst->getPointer<__wasi_array_output_t *>(EncapsulatedSecretPtr);
  checkExist(EncapsulatedSecret);

  if (auto Res = Ctx.kxEncapsulate(PkHandle); unlikely(!Res)) {
    return Res.error();
  } else {
    std::tie(*Secret, *EncapsulatedSecret) = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> Decapsulate::body(Runtime::Instance::MemoryInstance *MemInst,
                                   int32_t SkHandle,
                                   uint32_t EncapsulatedSecretPtr,
                                   uint32_t EncapsulatedSecretLen,
                                   uint32_t /* Out */ SecretPtr) {
  checkExist(MemInst);

  const __wasi_size_t WasiEncapsulatedSecretLen = EncapsulatedSecretLen;
  auto *const EncapsulatedSecret = MemInst->getPointer<const uint8_t *>(
      EncapsulatedSecretPtr, WasiEncapsulatedSecretLen);

  checkExist(EncapsulatedSecret);

  auto *const Secret = MemInst->getPointer<__wasi_array_output_t *>(SecretPtr);
  checkExist(Secret);

  if (auto Res = Ctx.kxDecapsulate(
          SkHandle, {EncapsulatedSecret, WasiEncapsulatedSecretLen});
      unlikely(!Res)) {
    return Res.error();
  } else {
    *Secret = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
