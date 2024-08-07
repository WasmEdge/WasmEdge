// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "kx/func.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

Expect<uint32_t> Dh::body(const Runtime::CallingFrame &Frame, int32_t PkHandle,
                          int32_t SkHandle,
                          uint32_t /* Out */ SharedSecretPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
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

Expect<uint32_t> Encapsulate::body(const Runtime::CallingFrame &Frame,
                                   int32_t PkHandle,
                                   uint32_t /* Out */ SecretPtr,
                                   uint32_t /* Out */ EncapsulatedSecretPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
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

Expect<uint32_t> Decapsulate::body(const Runtime::CallingFrame &Frame,
                                   int32_t SkHandle,
                                   uint32_t EncapsulatedSecretPtr,
                                   uint32_t EncapsulatedSecretLen,
                                   uint32_t /* Out */ SecretPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiEncapsulatedSecretLen = EncapsulatedSecretLen;
  const auto EncapsulatedSecret = MemInst->getSpan<const uint8_t>(
      EncapsulatedSecretPtr, WasiEncapsulatedSecretLen);

  checkRangeExist(EncapsulatedSecret, WasiEncapsulatedSecretLen);

  auto *const Secret = MemInst->getPointer<__wasi_array_output_t *>(SecretPtr);
  checkExist(Secret);

  if (auto Res = Ctx.kxDecapsulate(SkHandle, EncapsulatedSecret);
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
