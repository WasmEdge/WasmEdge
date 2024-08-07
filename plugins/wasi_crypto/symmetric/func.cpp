// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
  const auto Alg = MemInst->getStringView(AlgPtr, WasiAlgLen);
  checkRangeExist(Alg, WasiAlgLen);

  Algorithm WasiAlg;
  if (auto Res = tryFrom<Algorithm>(Alg); !Res) {
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
  const auto Alg = MemInst->getStringView(AlgPtr, WasiAlgLen);
  checkRangeExist(Alg, WasiAlgLen);

  Algorithm WasiAlg;
  if (auto Res = tryFrom<Algorithm>(Alg); !Res) {
    return Res.error();
  } else {
    WasiAlg = *Res;
  }

  auto *const Key = MemInst->getPointer<__wasi_symmetric_key_t *>(KeyPtr);
  checkExist(Key);

  const __wasi_size_t WasiRawLen = RawLen;
  const auto Raw = MemInst->getSpan<const uint8_t>(RawPtr, WasiRawLen);
  checkRangeExist(Raw, WasiRawLen);

  if (auto Res = Ctx.symmetricKeyImport(WasiAlg, Raw); unlikely(!Res)) {
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
  const auto Alg = MemInst->getStringView(AlgPtr, WasiAlgLen);
  checkRangeExist(Alg, WasiAlgLen);

  Algorithm WasiAlg;
  if (auto Res = tryFrom<Algorithm>(Alg); !Res) {
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
  const auto KeyId = MemInst->getSpan<uint8_t>(KeyIdPtr, WasiKeyIdMaxLen);
  checkRangeExist(KeyId, WasiKeyIdMaxLen);

  if (auto Res =
          Ctx.symmetricKeyStoreManaged(SecretsManagerHandle, KeyHandle, KeyId);
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
  const auto KeyId = MemInst->getSpan<uint8_t>(KeyIdPtr, WasiKeyIdMaxLen);
  checkRangeExist(KeyId, WasiKeyIdMaxLen);

  auto *const Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  checkExist(Size);

  auto *const KeyVersion =
      MemInst->getPointer<__wasi_version_t *>(KeyVersionPtr);
  if (unlikely(KeyVersion == nullptr)) {
    return __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE;
  }

  if (auto Res = Ctx.symmetricKeyId(KeyHandle, KeyId); unlikely(!Res)) {
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
  const auto KeyId = MemInst->getSpan<uint8_t>(KeyIdPtr, WasiKeyIdLen);
  checkRangeExist(KeyId, WasiKeyIdLen);

  auto *const KeyHandle =
      MemInst->getPointer<__wasi_symmetric_key_t *>(KeyHandlePtr);
  checkExist(KeyHandle);

  if (auto Res =
          Ctx.symmetricKeyFromId(SecretsManagerHandle, KeyId, KeyVersion);
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
  const auto Alg = MemInst->getStringView(AlgPtr, WasiAlgLen);
  checkRangeExist(Alg, WasiAlgLen);
  Algorithm WasiAlg;
  if (auto Res = tryFrom<Algorithm>(Alg); !Res) {
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
  const auto Name = MemInst->getStringView(NamePtr, WasiNameLen);
  checkRangeExist(Name, WasiNameLen);

  const __wasi_size_t WasiValueLen = ValueLen;
  const auto Value = MemInst->getSpan<uint8_t>(ValuePtr, WasiValueLen);
  checkRangeExist(Value, WasiValueLen);

  auto *const Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  checkExist(Size);

  if (auto Res = Ctx.symmetricStateOptionsGet(StateHandle, Name, Value)
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
  const auto Name = MemInst->getStringView(NamePtr, WasiNameLen);
  checkRangeExist(Name, WasiNameLen);

  auto *const U64 = MemInst->getPointer<uint64_t *>(U64Ptr);
  checkExist(U64);

  if (auto Res = Ctx.symmetricStateOptionsGetU64(StateHandle, Name);
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
  const auto Data = MemInst->getSpan<const uint8_t>(DataPtr, WasiDataLen);
  checkRangeExist(Data, WasiDataLen);

  if (auto Res = Ctx.symmetricStateAbsorb(StateHandle, Data); unlikely(!Res)) {
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
  const auto Out = MemInst->getSpan<uint8_t>(OutPtr, WasiOutLen);
  checkRangeExist(Out, WasiOutLen);

  if (auto Res = Ctx.symmetricStateSqueeze(StateHandle, Out); unlikely(!Res)) {
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
  const auto Alg = MemInst->getStringView(AlgPtr, WasiAlgLen);
  checkRangeExist(Alg, WasiAlgLen);
  Algorithm WasiAlg;
  if (auto Res = tryFrom<Algorithm>(Alg); !Res) {
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
  const auto Out = MemInst->getSpan<uint8_t>(OutPtr, WasiOutLen);
  checkRangeExist(Out, WasiOutLen);

  const __wasi_size_t WasiDataLen = DataLen;
  const auto Data = MemInst->getSpan<const uint8_t>(DataPtr, WasiDataLen);
  checkRangeExist(Data, WasiDataLen);

  auto *const Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  checkExist(Size);

  if (auto Res = Ctx.symmetricStateEncrypt(StateHandle, Out, Data)
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
  const auto Out = MemInst->getSpan<uint8_t>(OutPtr, WasiOutLen);
  checkRangeExist(Out, WasiOutLen);

  const __wasi_size_t WasiDataLen = DataLen;
  const auto Data = MemInst->getSpan<const uint8_t>(DataPtr, WasiDataLen);
  checkRangeExist(Data, WasiDataLen);

  auto *const TagHandle =
      MemInst->getPointer<__wasi_symmetric_tag_t *>(TagHandlePtr);
  checkExist(TagHandle);

  if (auto Res = Ctx.symmetricStateEncryptDetached(StateHandle, Out, Data);
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
  const auto Out = MemInst->getSpan<uint8_t>(OutPtr, WasiOutLen);
  checkRangeExist(Out, WasiOutLen);

  const __wasi_size_t WasiDataLen = DataLen;
  const auto Data = MemInst->getSpan<const uint8_t>(DataPtr, WasiDataLen);
  checkRangeExist(Data, WasiDataLen);

  auto *const Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  if (unlikely(Size == nullptr)) {
    return __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE;
  }

  if (auto Res = Ctx.symmetricStateDecrypt(StateHandle, Out, Data)
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
  const auto Out = MemInst->getSpan<uint8_t>(OutPtr, WasiOutLen);
  checkRangeExist(Out, WasiOutLen);

  const __wasi_size_t WasiDataLen = DataLen;
  const auto Data = MemInst->getSpan<const uint8_t>(DataPtr, WasiDataLen);
  checkRangeExist(Data, WasiDataLen);

  const __wasi_size_t WasiRawTagLen = RawTagLen;
  const auto RawTag = MemInst->getSpan<const uint8_t>(RawTagPtr, WasiRawTagLen);
  checkRangeExist(RawTag, WasiRawTagLen);

  auto *Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  checkExist(Size);

  if (auto Res =
          Ctx.symmetricStateDecryptDetached(StateHandle, Out, Data, RawTag)
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
  const auto Buf = MemInst->getSpan<uint8_t>(BufPtr, WasiBufLen);
  checkRangeExist(Buf, WasiBufLen);

  auto *Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  checkExist(Size);

  if (auto Res = Ctx.symmetricTagPull(TagHandle, Buf).and_then(toWasiSize);
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
  const auto RawTag = MemInst->getSpan<const uint8_t>(RawTagPtr, WasiRawTagLen);
  checkRangeExist(RawTag, WasiRawTagLen);

  if (auto Res = Ctx.symmetricTagVerify(TagHandle, RawTag); unlikely(!Res)) {
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
