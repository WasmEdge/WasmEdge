// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/key_exchange/func.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Kx {
Expect<uint32_t> Dh::body(Runtime::Instance::MemoryInstance *MemInst,
                          __wasi_publickey_t Pk, __wasi_secretkey_t Sk,
                          uint8_t_ptr /* Out */ ArrayOutputPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.dh(Pk, Sk);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *ArrayOutput =
      MemInst->getPointer<__wasi_array_output_t *>(ArrayOutputPtr);
  if (unlikely(ArrayOutput == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *ArrayOutput = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> Encapsulate::body(Runtime::Instance::MemoryInstance *MemInst,
                                   __wasi_publickey_t Pk,
                                   uint8_t_ptr /* Out */ ArrayOutput1Ptr,
                                   uint8_t_ptr /* Out */ ArrayOutput2Ptr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.encapsulate(Pk);
  if (unlikely(!Res)) {
    return Res.error();
  }
  auto [ArrayOutput1Res, ArrayOutput2Res] = *Res;

  auto *ArrayOutput1 =
      MemInst->getPointer<__wasi_array_output_t *>(ArrayOutput1Ptr);
  if (unlikely(ArrayOutput1 == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *ArrayOutput1 = ArrayOutput1Res;

  auto *ArrayOutput2 =
      MemInst->getPointer<__wasi_array_output_t *>(ArrayOutput2Ptr);
  if (unlikely(ArrayOutput2 == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *ArrayOutput2 = ArrayOutput2Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> Decapsulate::body(Runtime::Instance::MemoryInstance *MemInst,
                                   __wasi_secretkey_t Sk,
                                   const_uint8_t_ptr EncapsulatedSecret,
                                   __wasi_size_t EncapsulatedSecretLen,
                                   uint8_t_ptr /* Out */ ArrayOutputPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *EncapsulatedSecretMem =
      MemInst->getPointer<uint8_t *>(EncapsulatedSecret, EncapsulatedSecretLen);

  auto Res =
      Ctx.decapsulate(Sk, {EncapsulatedSecretMem, EncapsulatedSecretLen});
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *ArrayOutput =
      MemInst->getPointer<__wasi_array_output_t *>(ArrayOutputPtr);
  if (unlikely(ArrayOutput == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *ArrayOutput = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

} // namespace Kx
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
