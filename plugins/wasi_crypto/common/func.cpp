// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/func.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Common {

Expect<uint32_t> ArrayOutputLen::body(const Runtime::CallingFrame &Frame,
                                      int32_t ArrayOutputHandle,
                                      uint32_t /* Out */ SizePtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  auto *const Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  checkExist(Size);

  if (auto Res = Ctx.arrayOutputLen(ArrayOutputHandle).and_then(toWasiSize);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *Size = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> ArrayOutputPull::body(const Runtime::CallingFrame &Frame,
                                       int32_t ArrayOutputHandle,
                                       uint32_t BufPtr, uint32_t BufLen,
                                       uint32_t /* Out */ SizePtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiBufLen = BufLen;
  auto *const Buf = MemInst->getPointer<uint8_t *>(BufPtr, WasiBufLen);
  checkExist(Buf);

  auto *const Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  checkExist(Size);

  if (auto Res = Ctx.arrayOutputPull(ArrayOutputHandle, {Buf, WasiBufLen})
                     .and_then(toWasiSize);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *Size = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> OptionsOpen::body(const Runtime::CallingFrame &Frame,
                                   uint32_t AlgType,
                                   uint32_t /* Out */ OptionsHandlePtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  __wasi_algorithm_type_e_t WasiAlgType;
  if (auto Res = cast<__wasi_algorithm_type_e_t>(AlgType); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiAlgType = *Res;
  }

  auto *const OptionsHandle =
      MemInst->getPointer<__wasi_options_t *>(OptionsHandlePtr);
  checkExist(OptionsHandle);

  if (auto Res = Ctx.optionsOpen(WasiAlgType); unlikely(!Res)) {
    return Res.error();
  } else {
    *OptionsHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> OptionsClose::body(const Runtime::CallingFrame &,
                                    int32_t OptionsHandle) {
  if (auto Res = Ctx.optionsClose(OptionsHandle); unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> OptionsSet::body(const Runtime::CallingFrame &Frame,
                                  int32_t OptionsHandle, uint32_t NamePtr,
                                  uint32_t NameLen, uint32_t ValuePtr,
                                  uint32_t ValueLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiNameLen = NameLen;
  auto *const Name = MemInst->getPointer<const char *>(NamePtr, WasiNameLen);
  checkExist(Name);

  const __wasi_size_t WasiValueLen = ValueLen;
  auto *const Value =
      MemInst->getPointer<const uint8_t *>(ValuePtr, WasiValueLen);
  checkExist(Value);

  if (auto Res = Ctx.optionsSet(OptionsHandle, {Name, WasiNameLen},
                                {Value, WasiValueLen});
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> OptionsSetU64::body(const Runtime::CallingFrame &Frame,
                                     int32_t OptionsHandle, uint32_t NamePtr,
                                     uint32_t NameLen, uint64_t Value) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiNameLen = NameLen;
  auto *const Name = MemInst->getPointer<const char *>(NamePtr, WasiNameLen);
  checkExist(Name);

  if (auto Res = Ctx.optionsSetU64(OptionsHandle, {Name, WasiNameLen}, Value);
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> OptionsSetGuestBuffer::body(const Runtime::CallingFrame &Frame,
                                             int32_t OptionsHandle,
                                             uint32_t NamePtr, uint32_t NameLen,
                                             uint32_t BufPtr, uint32_t BufLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiNameLen = NameLen;
  auto *const Name = MemInst->getPointer<const char *>(NamePtr, WasiNameLen);
  checkExist(Name);

  const __wasi_size_t WasiBufLen = BufLen;
  auto *const Buf = MemInst->getPointer<uint8_t *>(BufPtr, WasiBufLen);
  checkExist(Buf);

  if (auto Res = Ctx.optionsSetGuestBuffer(OptionsHandle, {Name, WasiNameLen},
                                           {Buf, WasiBufLen});
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
SecretsManagerOpen::body(const Runtime::CallingFrame &Frame,
                         uint32_t OptOptionsHandlePtr,
                         uint32_t /* Out */ SecretsManagerHandlePtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  auto *const OptOptionsHandle =
      MemInst->getPointer<const __wasi_opt_options_t *>(OptOptionsHandlePtr);
  checkExist(OptOptionsHandle);

  auto *const SecretsManagerHandle =
      MemInst->getPointer<__wasi_secrets_manager_t *>(SecretsManagerHandlePtr);
  checkExist(SecretsManagerHandle);

  if (auto Res = Ctx.secretsManagerOpen(*OptOptionsHandle); unlikely(!Res)) {
    return Res.error();
  } else {
    *SecretsManagerHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> SecretsManagerClose::body(const Runtime::CallingFrame &,
                                           int32_t SecretsManagerHandle) {
  if (auto Res = Ctx.secretsManagerClose(SecretsManagerHandle);
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
SecretsManagerInvalidate::body(const Runtime::CallingFrame &Frame,
                               int32_t SecretsManagerHandle, uint32_t KeyIdPtr,
                               uint32_t KeyIdLen, uint64_t Version) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiKeyIdLen = KeyIdLen;
  auto *const KeyId =
      MemInst->getPointer<const uint8_t *>(KeyIdPtr, WasiKeyIdLen);
  checkExist(KeyId);

  if (auto Res = Ctx.secretsManagerInvalidate(SecretsManagerHandle,
                                              {KeyId, WasiKeyIdLen}, Version);
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

} // namespace Common
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
