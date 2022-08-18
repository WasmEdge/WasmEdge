// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "symmetric/func.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

Expect<uint32_t> KeyGenerate::body(const Runtime::CallingFrame &Frame,
                                   uint32_t AlgPtr, uint32_t AlgLen,
                                   uint32_t OptOptionsPtr,
                                   uint32_t /* Out */ KeyHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiAlgLen = AlgLen;
  auto *const Alg = MemInst->getPointer<const char *>(AlgPtr, WasiAlgLen);
  checkExist(Alg);
  Algorithm WasiAlg;
  if (auto Res = tryFrom<Algorithm>({Alg, WasiAlgLen}); !Res) {
    return Res.error();
  } else {
    WasiAlg = *Res;
  }

  auto *const OptOptions =
      MemInst->getPointer<const __wasi_opt_options_t *>(OptOptionsPtr);
  checkExist(OptOptions);

  auto *const KeyHandle =
      MemInst->getPointer<__wasi_symmetric_key_t *>(KeyHandlePtr);
  checkExist(KeyHandle);

  if (auto Res = Ctx.symmetricKeyGenerate(WasiAlg, *OptOptions);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *KeyHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeyImport::body(const Runtime::CallingFrame &Frame,
                                 uint32_t AlgPtr, uint32_t AlgLen,
                                 uint32_t RawPtr, uint32_t RawLen,
                                 uint32_t /* Out */ KeyPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiAlgLen = AlgLen;
  auto *const Alg = MemInst->getPointer<const char *>(AlgPtr, WasiAlgLen);
  checkExist(Alg);

  Algorithm WasiAlg;
  if (auto Res = tryFrom<Algorithm>({Alg, WasiAlgLen}); !Res) {
    return Res.error();
  } else {
    WasiAlg = *Res;
  }

  auto *const Key = MemInst->getPointer<__wasi_symmetric_key_t *>(KeyPtr);
  checkExist(Key);

  const __wasi_size_t WasiRawLen = RawLen;
  auto *Raw = MemInst->getPointer<const uint8_t *>(RawPtr, WasiRawLen);
  checkExist(Raw);

  if (auto Res = Ctx.symmetricKeyImport(WasiAlg, {Raw, WasiRawLen});
      unlikely(!Res)) {
    return Res.error();
  } else {
    *Key = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeyExport::body(const Runtime::CallingFrame &Frame,
                                 int32_t KeyHandle,
                                 uint32_t /* Out */ ArrayOutputHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  auto *const ArrayOutputHandle =
      MemInst->getPointer<__wasi_array_output_t *>(ArrayOutputHandlePtr);
  checkExist(ArrayOutputHandle);

  if (auto Res = Ctx.symmetricKeyExport(KeyHandle); unlikely(!Res)) {
    return Res.error();
  } else {
    *ArrayOutputHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeyClose::body(const Runtime::CallingFrame &,
                                int32_t KeyHandle) {
  if (auto Res = Ctx.symmetricKeyClose(KeyHandle); unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeyGenerateManaged::body(const Runtime::CallingFrame &Frame,
                                          int32_t SecretsManagerHandle,
                                          uint32_t AlgPtr, uint32_t AlgLen,
                                          uint32_t OptOptionsPtr,
                                          uint32_t /* Out */ KeyHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiAlgLen = AlgLen;
  auto *const Alg = MemInst->getPointer<const char *>(AlgPtr, WasiAlgLen);
  checkExist(Alg);
  Algorithm WasiAlg;
  if (auto Res = tryFrom<Algorithm>({Alg, WasiAlgLen}); !Res) {
    return Res.error();
  } else {
    WasiAlg = *Res;
  }

  auto *const OptOptions =
      MemInst->getPointer<const __wasi_opt_options_t *>(OptOptionsPtr);
  checkExist(OptOptions);

  auto *const KeyHandle =
      MemInst->getPointer<__wasi_symmetric_key_t *>(KeyHandlePtr);
  checkExist(KeyHandle);

  if (auto Res = Ctx.symmetricKeyGenerateManaged(SecretsManagerHandle, WasiAlg,
                                                 *OptOptions);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *KeyHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeyStoreManaged::body(const Runtime::CallingFrame &Frame,
                                       int32_t SecretsManagerHandle,
                                       int32_t KeyHandle, uint32_t KeyIdPtr,
                                       uint32_t KeyIdMaxLen) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiKeyIdMaxLen = KeyIdMaxLen;
  auto *const KeyId = MemInst->getPointer<uint8_t *>(KeyIdPtr, WasiKeyIdMaxLen);
  checkExist(KeyId);

  if (auto Res = Ctx.symmetricKeyStoreManaged(SecretsManagerHandle, KeyHandle,
                                              {KeyId, WasiKeyIdMaxLen});
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeyReplaceManaged::body(const Runtime::CallingFrame &Frame,
                                         int32_t SecretsManagerHandle,
                                         int32_t OldKeyHandle,
                                         int32_t NewKeyHandle,
                                         uint32_t /* Out */ KeyVersionPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  auto *const KeyVersion =
      MemInst->getPointer<__wasi_version_t *>(KeyVersionPtr);
  checkExist(KeyVersion);

  if (auto Res = Ctx.symmetricKeyReplaceManaged(SecretsManagerHandle,
                                                OldKeyHandle, NewKeyHandle);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *KeyVersion = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeyId::body(const Runtime::CallingFrame &Frame,
                             int32_t KeyHandle, uint32_t KeyIdPtr,
                             uint32_t KeyIdMaxLen, uint32_t /* Out */ SizePtr,
                             uint32_t /* Out */ KeyVersionPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiKeyIdMaxLen = KeyIdMaxLen;
  auto *const KeyId = MemInst->getPointer<uint8_t *>(KeyIdPtr, WasiKeyIdMaxLen);
  checkExist(KeyId);

  auto *const Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  checkExist(Size);

  auto *const KeyVersion =
      MemInst->getPointer<__wasi_version_t *>(KeyVersionPtr);
  if (unlikely(KeyVersion == nullptr)) {
    return __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE;
  }

  if (auto Res = Ctx.symmetricKeyId(KeyHandle, {KeyId, WasiKeyIdMaxLen});
      unlikely(!Res)) {
    return Res.error();
  } else {
    auto [SizeRes, VersionRes] = *Res;
    auto SafeSizeRes = toWasiSize(SizeRes);
    if (unlikely(!SafeSizeRes)) {
      return SafeSizeRes.error();
    }

    *KeyVersion = VersionRes;
    *Size = *SafeSizeRes;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeyFromId::body(const Runtime::CallingFrame &Frame,
                                 int32_t SecretsManagerHandle,
                                 uint32_t KeyIdPtr, uint32_t KeyIdLen,
                                 uint64_t KeyVersion,
                                 uint32_t /* Out */ KeyHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiKeyIdLen = KeyIdLen;
  auto *const KeyId = MemInst->getPointer<uint8_t *>(KeyIdPtr, WasiKeyIdLen);
  checkExist(KeyId);

  auto *const KeyHandle =
      MemInst->getPointer<__wasi_symmetric_key_t *>(KeyHandlePtr);
  checkExist(KeyHandle);

  if (auto Res = Ctx.symmetricKeyFromId(SecretsManagerHandle,
                                        {KeyId, WasiKeyIdLen}, KeyVersion);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *KeyHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateOpen::body(const Runtime::CallingFrame &Frame,
                                 uint32_t AlgPtr, uint32_t AlgLen,
                                 uint32_t OptKeyHandlePtr,
                                 uint32_t OptOptionsPtr,
                                 uint32_t /* Out */ StatePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiAlgLen = AlgLen;
  auto *const Alg = MemInst->getPointer<const char *>(AlgPtr, WasiAlgLen);
  checkExist(Alg);
  Algorithm WasiAlg;
  if (auto Res = tryFrom<Algorithm>({Alg, WasiAlgLen}); !Res) {
    return Res.error();
  } else {
    WasiAlg = *Res;
  }

  auto *const OptKeyHandle =
      MemInst->getPointer<const __wasi_opt_symmetric_key_t *>(OptKeyHandlePtr);
  checkExist(OptKeyHandle);

  auto *const OptOptions =
      MemInst->getPointer<const __wasi_opt_options_t *>(OptOptionsPtr);
  checkExist(OptOptions);

  auto *const State = MemInst->getPointer<__wasi_symmetric_state_t *>(StatePtr);
  if (unlikely(State == nullptr)) {
    return __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE;
  }

  if (auto Res = Ctx.symmetricStateOpen(WasiAlg, *OptKeyHandle, *OptOptions);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *State = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateClone::body(const Runtime::CallingFrame &Frame,
                                  int32_t StateHandle,
                                  uint32_t /* Out */ StatePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  auto *const State = MemInst->getPointer<__wasi_symmetric_state_t *>(StatePtr);
  if (unlikely(State == nullptr)) {
    return __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE;
  }

  if (auto Res = Ctx.symmetricStateClone(StateHandle); unlikely(!Res)) {
    return Res.error();
  } else {
    *State = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateOptionsGet::body(const Runtime::CallingFrame &Frame,
                                       int32_t StateHandle, uint32_t NamePtr,
                                       uint32_t NameLen, uint32_t ValuePtr,
                                       uint32_t ValueLen,
                                       uint32_t /* Out */ SizePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiNameLen = NameLen;
  auto *const Name = MemInst->getPointer<const char *>(NamePtr, WasiNameLen);
  checkExist(Name);

  const __wasi_size_t WasiValueLen = ValueLen;
  auto *const Value = MemInst->getPointer<uint8_t *>(ValuePtr, ValueLen);
  checkExist(Value);

  auto *const Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  checkExist(Size);

  if (auto Res = Ctx.symmetricStateOptionsGet(StateHandle, {Name, WasiNameLen},
                                              {Value, WasiValueLen})
                     .and_then(toWasiSize);
      !Res) {
    return Res.error();
  } else {
    *Size = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateOptionsGetU64::body(const Runtime::CallingFrame &Frame,
                                          int32_t StateHandle, uint32_t NamePtr,
                                          uint32_t NameLen,
                                          uint32_t /* Out */ U64Ptr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiNameLen = NameLen;
  auto *const Name = MemInst->getPointer<const char *>(NamePtr, WasiNameLen);
  checkExist(Name);

  auto *const U64 = MemInst->getPointer<uint64_t *>(U64Ptr);
  checkExist(U64);

  if (auto Res =
          Ctx.symmetricStateOptionsGetU64(StateHandle, {Name, WasiNameLen});
      unlikely(!Res)) {
    return Res.error();
  } else {
    *U64 = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateClose::body(const Runtime::CallingFrame &,
                                  int32_t StateHandle) {
  if (auto Res = Ctx.symmetricStateClose(StateHandle); unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateAbsorb::body(const Runtime::CallingFrame &Frame,
                                   int32_t StateHandle, uint32_t DataPtr,
                                   uint32_t DataLen) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiDataLen = DataLen;
  auto *const Data = MemInst->getPointer<const uint8_t *>(DataPtr, WasiDataLen);
  checkExist(Data);

  if (auto Res = Ctx.symmetricStateAbsorb(StateHandle, {Data, WasiDataLen});
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateSqueeze::body(const Runtime::CallingFrame &Frame,
                                    int32_t StateHandle, uint32_t OutPtr,
                                    uint32_t OutLen) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiOutLen = OutLen;
  auto *const Out = MemInst->getPointer<uint8_t *>(OutPtr, WasiOutLen);
  checkExist(Out);

  if (auto Res = Ctx.symmetricStateSqueeze(StateHandle, {Out, WasiOutLen});
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateSqueezeTag::body(const Runtime::CallingFrame &Frame,
                                       int32_t StateHandle,
                                       uint32_t /* Out */ TagHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  auto *const TagHandle =
      MemInst->getPointer<__wasi_symmetric_tag_t *>(TagHandlePtr);
  checkExist(TagHandle);

  if (auto Res = Ctx.symmetricStateSqueezeTag(StateHandle); unlikely(!Res)) {
    return Res.error();
  } else {
    *TagHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateSqueezeKey::body(const Runtime::CallingFrame &Frame,
                                       int32_t StateHandle, uint32_t AlgPtr,
                                       uint32_t AlgLen,
                                       uint32_t /* Out */ KeyHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiAlgLen = AlgLen;
  auto *const Alg = MemInst->getPointer<const char *>(AlgPtr, WasiAlgLen);
  checkExist(Alg);
  Algorithm WasiAlg;
  if (auto Res = tryFrom<Algorithm>({Alg, WasiAlgLen}); !Res) {
    return Res.error();
  } else {
    WasiAlg = *Res;
  }

  auto *const KeyHandle =
      MemInst->getPointer<__wasi_symmetric_key_t *>(KeyHandlePtr);
  checkExist(KeyHandle);

  if (auto Res = Ctx.symmetricStateSqueezeKey(StateHandle, WasiAlg);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *KeyHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateMaxTagLen::body(const Runtime::CallingFrame &Frame,
                                      int32_t StateHandle,
                                      uint32_t /* Out */ SizePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  auto *const Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  checkExist(Size);

  if (auto Res = Ctx.symmetricStateMaxTagLen(StateHandle).and_then(toWasiSize);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *Size = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateEncrypt::body(const Runtime::CallingFrame &Frame,
                                    int32_t StateHandle, uint32_t OutPtr,
                                    uint32_t OutLen, uint32_t DataPtr,
                                    uint32_t DataLen,
                                    uint32_t /* Out */ SizePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiOutLen = OutLen;
  auto *const Out = MemInst->getPointer<uint8_t *>(OutPtr, WasiOutLen);
  checkExist(Out);

  const __wasi_size_t WasiDataLen = DataLen;
  auto *Data = MemInst->getPointer<const uint8_t *>(DataPtr, WasiDataLen);
  checkExist(Data);

  auto *const Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  checkExist(Size);

  if (auto Res = Ctx.symmetricStateEncrypt(StateHandle, {Out, WasiOutLen},
                                           {Data, WasiDataLen})
                     .and_then(toWasiSize);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *Size = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateEncryptDetached::body(const Runtime::CallingFrame &Frame,
                                            int32_t StateHandle,
                                            uint32_t OutPtr, uint32_t OutLen,
                                            uint32_t DataPtr, uint32_t DataLen,
                                            uint32_t /* Out */ TagHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiOutLen = OutLen;
  auto *const Out = MemInst->getPointer<uint8_t *>(OutPtr, WasiOutLen);
  checkExist(Out);

  const __wasi_size_t WasiDataLen = DataLen;
  auto *const Data = MemInst->getPointer<const uint8_t *>(DataPtr, WasiDataLen);
  checkExist(Data);

  auto *const TagHandle =
      MemInst->getPointer<__wasi_symmetric_tag_t *>(TagHandlePtr);
  checkExist(TagHandle);

  if (auto Res = Ctx.symmetricStateEncryptDetached(
          StateHandle, {Out, WasiOutLen}, {Data, WasiDataLen});
      unlikely(!Res)) {
    return Res.error();
  } else {
    *TagHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateDecrypt::body(const Runtime::CallingFrame &Frame,
                                    int32_t StateHandle, uint32_t OutPtr,
                                    uint32_t OutLen, uint32_t DataPtr,
                                    uint32_t DataLen,
                                    uint32_t /* Out */ SizePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiOutLen = OutLen;
  auto *const Out = MemInst->getPointer<uint8_t *>(OutPtr, WasiOutLen);
  checkExist(Out);

  const __wasi_size_t WasiDataLen = DataLen;
  auto *const Data = MemInst->getPointer<const uint8_t *>(DataPtr, WasiDataLen);
  checkExist(Data);

  auto *const Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  if (unlikely(Size == nullptr)) {
    return __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE;
  }

  if (auto Res = Ctx.symmetricStateDecrypt(StateHandle, {Out, WasiOutLen},
                                           {Data, WasiDataLen})
                     .and_then(toWasiSize);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *Size = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateDecryptDetached::body(
    const Runtime::CallingFrame &Frame, int32_t StateHandle, uint32_t OutPtr,
    uint32_t OutLen, uint32_t DataPtr, uint32_t DataLen, uint32_t RawTagPtr,
    uint32_t RawTagLen, uint32_t /* Out */ SizePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiOutLen = OutLen;
  auto *const Out = MemInst->getPointer<uint8_t *>(OutPtr, WasiOutLen);
  checkExist(Out);

  const __wasi_size_t WasiDataLen = DataLen;
  auto *const Data = MemInst->getPointer<const uint8_t *>(DataPtr, WasiDataLen);
  checkExist(Data);

  const __wasi_size_t WasiRawTagLen = RawTagLen;
  auto *RawTag = MemInst->getPointer<const uint8_t *>(RawTagPtr, WasiRawTagLen);
  checkExist(RawTag);

  auto *Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  checkExist(Size);

  if (auto Res = Ctx.symmetricStateDecryptDetached(
                        StateHandle, {Out, WasiOutLen}, {Data, WasiDataLen},
                        {RawTag, WasiRawTagLen})
                     .and_then(toWasiSize);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *Size = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateRatchet::body(const Runtime::CallingFrame &,
                                    int32_t StateHandle) {
  if (auto Res = Ctx.symmetricStateRatchet(StateHandle); unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> TagLen::body(const Runtime::CallingFrame &Frame,
                              int32_t TagHandle, uint32_t /* Out */ SizePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  auto *Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  if (unlikely(Size == nullptr)) {
    return __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE;
  }

  if (auto Res = Ctx.symmetricTagLen(TagHandle).and_then(toWasiSize);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *Size = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> TagPull::body(const Runtime::CallingFrame &Frame,
                               int32_t TagHandle, uint32_t BufPtr,
                               uint32_t BufLen, uint32_t /* Out */ SizePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiBufLen = BufLen;
  auto *Buf = MemInst->getPointer<uint8_t *>(BufPtr, WasiBufLen);
  checkExist(Buf);

  auto *Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  checkExist(Size);

  if (auto Res = Ctx.symmetricTagPull(TagHandle, {Buf, WasiBufLen})
                     .and_then(toWasiSize);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *Size = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> TagVerify::body(const Runtime::CallingFrame &Frame,
                                 int32_t TagHandle, uint32_t RawTagPtr,
                                 uint32_t RawTagLen) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiRawTagLen = RawTagLen;
  auto *RawTag = MemInst->getPointer<const uint8_t *>(RawTagPtr, WasiRawTagLen);
  checkExist(RawTag);

  if (auto Res = Ctx.symmetricTagVerify(TagHandle, {RawTag, RawTagLen});
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> TagClose::body(const Runtime::CallingFrame &,
                                int32_t TagHandle) {
  if (auto Res = Ctx.symmetricTagClose(TagHandle); unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
