// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/common/func.h"
#include "common/errcode.h"
#include "host/wasi_crypto/util.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Common {

Expect<uint32_t>
ArrayOutputLen::body(Runtime::Instance::MemoryInstance *MemInst,
                     __wasi_array_output_t ArrayOutputHandle,
                     uint32_t /* Out */ SizePtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Len = Ctx.arrayOutputLen(ArrayOutputHandle);
  if (unlikely(!Len)) {
    return Len.error();
  }

  auto SafeLen = cast<__wasi_size_t>(*Len);
  if (unlikely(!SafeLen)) {
    return SafeLen.error();
  }

  auto *const Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  if (unlikely(Size == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Size = *SafeLen;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
ArrayOutputPull::body(Runtime::Instance::MemoryInstance *MemInst,
                      __wasi_array_output_t ArrayOutputHandle,
                      uint8_t_ptr BufPtr, __wasi_size_t BufLen,
                      uint32_t /* Out */ SizePtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *BufMem = MemInst->getPointer<uint8_t *>(BufPtr, BufLen);

  if (unlikely(BufMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  Span<uint8_t> Buf{BufMem, BufLen};

  auto Len = Ctx.arrayOutputPull(ArrayOutputHandle, Buf);
  if (unlikely(!Len)) {
    return Len.error();
  }

  auto SafeLen = cast<__wasi_size_t>(*Len);
  if (unlikely(!SafeLen)) {
    return SafeLen.error();
  }

  auto *const Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  if (unlikely(Size == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Size = *SafeLen;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> OptionsOpen::body(Runtime::Instance::MemoryInstance *MemInst,
                                   uint32_t AlgorithmType,
                                   uint32_t /* Out */ OptionsPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  __wasi_algorithm_type_e_t Algorithm;
  if (auto Res = cast<__wasi_algorithm_type_e_t>(AlgorithmType);
      unlikely(!Res)) {
    return Res.error();
  } else {
    Algorithm = *Res;
  }

  auto *const Options = MemInst->getPointer<__wasi_options_t *>(OptionsPtr);
  if (unlikely(Options == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  if (auto Res = Ctx.optionsOpen(Algorithm); unlikely(!Res)) {
    return Res.error();
  } else {
    *Options = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> OptionsClose::body(Runtime::Instance::MemoryInstance *MemInst,
                                    __wasi_options_t Handle) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.optionsClose(Handle);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> OptionsSet::body(Runtime::Instance::MemoryInstance *MemInst,
                                  __wasi_options_t Handle,
                                  const_uint8_t_ptr NamePtr,
                                  __wasi_size_t NameLen,
                                  const_uint8_t_ptr ValuePtr,
                                  __wasi_size_t ValueLen) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *ValueMem = MemInst->getPointer<uint8_t const *>(ValuePtr, ValueLen);
  if (unlikely(ValueMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t const> Value{ValueMem, ValueLen};
  auto *const NameMem = MemInst->getPointer<const char *>(NamePtr, NameLen);

  if (unlikely(ValueMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.optionsSet(Handle, {NameMem, NameLen}, Value);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> OptionsSetU64::body(Runtime::Instance::MemoryInstance *MemInst,
                                     __wasi_options_t Handle,
                                     const_uint8_t_ptr NamePtr,
                                     __wasi_size_t NameLen, uint64_t Value) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *const NameMem = MemInst->getPointer<const char *>(NamePtr, NameLen);
  if (unlikely(NameMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.optionsSetU64(Handle, {NameMem, NameLen}, Value);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
OptionsSetGuestBuffer::body(Runtime::Instance::MemoryInstance *MemInst,
                            __wasi_options_t Handle, const_uint8_t_ptr NamePtr,
                            __wasi_size_t NameLen, uint8_t_ptr BufPtr,
                            __wasi_size_t BufLen) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *const NameMem = MemInst->getPointer<const char *>(NamePtr, NameLen);

  if (unlikely(NameMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *BufMem = MemInst->getPointer<uint8_t *>(BufPtr, BufLen);
  if (unlikely(BufMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  Span<uint8_t> Buf{BufMem, BufLen};

  auto Res = Ctx.optionsSetGuestBuffer(Handle, {NameMem, NameLen}, Buf);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
SecretsMangerOpen::body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t OptOptionsPtr,
                        uint32_t /* Out */ SecretsManagerPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *const OptOption =
      MemInst->getPointer<__wasi_opt_options_t *>(OptOptionsPtr);

  if (unlikely(OptOption == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.secretsMangerOpen(parseCUnion<__wasi_options_t>(*OptOption));
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *const SecretsManager =
      MemInst->getPointer<__wasi_secrets_manager_t *>(SecretsManagerPtr);
  if (unlikely(SecretsManager == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *SecretsManager = *Res;
  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
SecretsMangerClose::body(Runtime::Instance::MemoryInstance *MemInst,
                         __wasi_secrets_manager_t SecretsManger) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.secretsMangerClose(SecretsManger);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> SecretsMangerInvalidate::body(
    Runtime::Instance::MemoryInstance *MemInst,
    __wasi_secrets_manager_t SecretsManger, const_uint8_t_ptr KeyIdPtr,
    __wasi_size_t KeyIdLen, __wasi_version_t Version) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *const KeyIdMem =
      MemInst->getPointer<uint8_t const *>(KeyIdPtr, KeyIdLen);

  if (unlikely(KeyIdMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  Span<uint8_t const> KeyId{KeyIdMem, KeyIdLen};

  auto Res = Ctx.secretsManagerInvalidate(SecretsManger, KeyId, Version);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

} // namespace Common
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
