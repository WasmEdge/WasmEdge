// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/func.h"
#include "host/wasi_crypto/util.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {

Expect<uint32_t>
WasiSymmetricKeyGenerate::body(Runtime::Instance::MemoryInstance *MemInst,
                               const_uint8_t_ptr AlgPtr,
                               __wasi_size_t AlgLen, uint32_t OptionsPtr,
                               uint32_t /* Out */ KeyPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *AlgMem = MemInst->getPointer<const char *>(AlgPtr, AlgLen);
  if (unlikely(AlgMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  std::string_view Alg{AlgMem, AlgLen};

  auto *const OptOption =
      MemInst->getPointer<__wasi_opt_options_t *>(OptionsPtr);
  if (unlikely(OptOption == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.symmetricKeyGenerate(
      Alg, WASICrypto::parseCUnion<__wasi_options_t>(*OptOption));

  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *const Key = MemInst->getPointer<__wasi_symmetric_key_t *>(KeyPtr);
  if (unlikely(Key == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  *Key = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSymmetricKeyImport::body(Runtime::Instance::MemoryInstance *MemInst,
                             const_uint8_t_ptr AlgPtr, __wasi_size_t AlgLen,
                             const_uint8_t_ptr RawPtr, __wasi_size_t RawLen,
                             uint32_t /* Out */ KeyPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *AlgMem = MemInst->getPointer<const char *>(AlgPtr, AlgLen);
  if (unlikely(AlgMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  std::string_view Alg{AlgMem, AlgLen};

  auto *RawMem = MemInst->getPointer<uint8_t const *>(RawPtr, RawLen);
  if (unlikely(RawMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t const> Raw{RawMem, RawLen};

  auto Res = Ctx.symmetricKeyImport(Alg, Raw);

  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *const Key = MemInst->getPointer<__wasi_symmetric_key_t *>(KeyPtr);
  if (unlikely(Key == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  *Key = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSymmetricKeyExport::body(Runtime::Instance::MemoryInstance *MemInst,
                             __wasi_symmetric_key_t SymmetricKey,
                             uint32_t /* Out */ ArrayOutputPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.symmetricKeyExport(SymmetricKey);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *const ArrayOutput =
      MemInst->getPointer<__wasi_array_output_t *>(ArrayOutputPtr);
  if (unlikely(ArrayOutput == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *ArrayOutput = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSymmetricKeyClose::body(Runtime::Instance::MemoryInstance *MemInst,
                            __wasi_symmetric_key_t SymmetricKey) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.symmetricKeyClose(SymmetricKey);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSymmetricKeyGenerateManaged::body(
    Runtime::Instance::MemoryInstance *MemInst,
    __wasi_secrets_manager_t SecretsManager, const_uint8_t_ptr AlgPtr,
    __wasi_size_t AlgLen, uint32_t OptOptionsPtr, uint32_t /* Out */ KeyPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *AlgMem = MemInst->getPointer<const char *>(AlgPtr, AlgLen);
  if (unlikely(AlgMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  std::string_view Alg{AlgMem, AlgLen};

  auto *OptOptions = MemInst->getPointer<__wasi_opt_options_t *>(OptOptionsPtr);
  if (unlikely(OptOptions == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.symmetricKeyGenerateManaged(
      SecretsManager, Alg,
      WASICrypto::parseCUnion<__wasi_options_t>(*OptOptions));
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *const Key = MemInst->getPointer<__wasi_symmetric_key_t *>(KeyPtr);
  if (unlikely(Key == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Key = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSymmetricKeyStoreManaged::body(Runtime::Instance::MemoryInstance *MemInst,
                                   __wasi_secrets_manager_t SecretsManager,
                                   __wasi_symmetric_key_t SymmetricKey,
                                   uint8_t_ptr SymmetricKeyIdPtr,
                                   __wasi_size_t SymmetricKeyIdMaxLen) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.symmetricKeyStoreManaged(
      SecretsManager, SymmetricKey, SymmetricKeyIdPtr, SymmetricKeyIdMaxLen);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSymmetricKeyReplaceManaged::body(Runtime::Instance::MemoryInstance *MemInst,
                                     __wasi_secrets_manager_t SecretsManager,
                                     __wasi_symmetric_key_t SymmetricKeyOld,
                                     __wasi_symmetric_key_t SymmetricKeyNew,
                                     uint32_t /* Out */ VersionPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.symmetricKeyReplaceManaged(SecretsManager, SymmetricKeyOld,
                                            SymmetricKeyNew);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *Version = MemInst->getPointer<__wasi_version_t *>(VersionPtr);
  if (unlikely(Version == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Version = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSymmetricKeyId::body(
    Runtime::Instance::MemoryInstance *MemInst,
    __wasi_symmetric_key_t SymmetricKey, uint8_t_ptr SymmetricKeyId,
    __wasi_size_t SymmetricKeyIdMaxLen, uint32_t /* Out */ SizePtr,
    uint32_t /* Out */ VersionPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res =
      Ctx.symmetricKeyId(SymmetricKey, SymmetricKeyId, SymmetricKeyIdMaxLen);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *Version = MemInst->getPointer<__wasi_version_t *>(VersionPtr);
  if (unlikely(Version == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *Size = MemInst->getPointer<__wasi_version_t *>(SizePtr);
  if (unlikely(Size == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto [SizeRes, VersionRes] = *Res;

  *Version = VersionRes;
  *Size = SizeRes;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSymmetricKeyFromId::body(
    Runtime::Instance::MemoryInstance *MemInst,
    __wasi_secrets_manager_t SecretsManager, uint8_t_ptr SymmetricKeyIdPtr,
    __wasi_size_t SymmetricKeyIdLen, __wasi_version_t SymmetricKeyVersion,
    uint32_t /* Out */ KeyPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *SymmetricKeyIdMem =
      MemInst->getPointer<uint8_t *>(SymmetricKeyIdPtr, SymmetricKeyIdLen);
  if (unlikely(SymmetricKeyIdMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t> SymmetricKeyId{SymmetricKeyIdMem, SymmetricKeyIdLen};

  auto Res = Ctx.symmetricKeyFromId(SecretsManager, SymmetricKeyId,
                                    SymmetricKeyVersion);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *Key = MemInst->getPointer<uint8_t *>(KeyPtr);

  *Key = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSymmetricStateOpen::body(Runtime::Instance::MemoryInstance *MemInst,
                             const_uint8_t_ptr AlgPtr, __wasi_size_t AlgLen,
                             uint32_t OptKeyPtr, uint32_t OptOptionsPtr,
                             uint32_t /* Out */ StatePtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *AlgMem = MemInst->getPointer<const char *>(AlgPtr, AlgLen);
  if (unlikely(AlgMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *OptKey = MemInst->getPointer<__wasi_opt_symmetric_key_t *>(OptKeyPtr);
  if (unlikely(OptKey == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *OptOptions = MemInst->getPointer<__wasi_opt_options_t *>(OptOptionsPtr);
  if (unlikely(OptOptions == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  std::string_view Name{AlgMem, AlgLen};

  auto Res = Ctx.symmetricStateOpen(
      Name, WASICrypto::parseCUnion<__wasi_symmetric_key_t>(*OptKey),
      WASICrypto::parseCUnion<__wasi_options_t>(*OptOptions));
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *State = MemInst->getPointer<uint8_t *>(StatePtr);
  if (unlikely(State == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *State = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSymmetricStateOptionsGet::body(
    Runtime::Instance::MemoryInstance *MemInst, __wasi_symmetric_state_t Handle,
    const_uint8_t_ptr NamePtr, __wasi_size_t NameLen, uint8_t_ptr ValuePtr,
    __wasi_size_t ValueLen, uint32_t /* Out */ SizePtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *NameMem = MemInst->getPointer<const char *>(NamePtr, NameLen);
  if (unlikely(NameMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  std::string_view Name{NameMem, NameLen};

  auto *ValueMem = MemInst->getPointer<uint8_t *>(ValuePtr, ValueLen);
  if (unlikely(ValueMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t> Value{ValueMem, ValueLen};

  auto Res = Ctx.symmetricStateOptionsGet(Handle, Name, Value);
  if (!Res) {
    return Res.error();
  }

  auto *Size = MemInst->getPointer<uint8_t *>(SizePtr);
  if (unlikely(Size == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  *Size = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSymmetricStateOptionsGetU64::body(
    Runtime::Instance::MemoryInstance *MemInst, __wasi_symmetric_state_t Handle,
    const_uint8_t_ptr NamePtr, __wasi_size_t NameLen,
    uint32_t /* Out */ U64Ptr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *NameMem = MemInst->getPointer<const char *>(NamePtr, NameLen);
  if (unlikely(NameMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  std::string_view Name{NameMem, NameLen};

  auto Res = Ctx.symmetricStateOptionsGetU64(Handle, Name);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *U64 = MemInst->getPointer<uint64_t *>(U64Ptr);
  *U64 = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSymmetricStateClose::body(Runtime::Instance::MemoryInstance *MemInst,
                              __wasi_symmetric_state_t Handle) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.symmetricStateClose(Handle);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSymmetricStateAbsorb::body(
    Runtime::Instance::MemoryInstance *MemInst, __wasi_symmetric_state_t Handle,
    const_uint8_t_ptr DataPtr, __wasi_size_t DataLen) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *DataMem = MemInst->getPointer<uint8_t const *>(DataPtr, DataLen);
  if (unlikely(DataMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t const> Data{DataMem, DataLen};

  auto Res = Ctx.symmetricStateAbsorb(Handle, Data);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSymmetricStateSqueeze::body(Runtime::Instance::MemoryInstance *MemInst,
                                __wasi_symmetric_state_t Handle,
                                uint8_t_ptr OutPtr, __wasi_size_t OutLen) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *OutMem = MemInst->getPointer<uint8_t const *>(OutPtr, OutLen);
  if (unlikely(OutMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t const> Out{OutMem, OutLen};

  auto Res = Ctx.symmetricStateAbsorb(Handle, Out);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSymmetricStateSqueezeTag::body(Runtime::Instance::MemoryInstance *MemInst,
                                   __wasi_symmetric_state_t Handle,
                                   uint32_t /* Out */ TagPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.symmetricStateSqueezeTag(Handle);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *Tag = MemInst->getPointer<__wasi_symmetric_tag_t *>(TagPtr);
  if (unlikely(Tag == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Tag = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSymmetricStateSqueezeKey::body(
    Runtime::Instance::MemoryInstance *MemInst, __wasi_symmetric_state_t Handle,
    const_uint8_t_ptr AlgPtr, __wasi_size_t AlgLen, uint32_t /* Out */ KeyPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  auto *AlgMem = MemInst->getPointer<const char *>(AlgPtr, AlgLen);
  if (unlikely(AlgMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  std::string_view Alg{AlgMem, AlgLen};

  auto Res = Ctx.symmetricStateSqueezeKey(Handle, Alg);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *Key = MemInst->getPointer<__wasi_symmetric_key_t *>(KeyPtr);
  if (unlikely(Key == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Key = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSymmetricStateMaxTagLen::body(Runtime::Instance::MemoryInstance *MemInst,
                                  __wasi_symmetric_state_t Handle,
                                  uint32_t /* Out */ SizePtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.symmetricStateMaxTagLen(Handle);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  if (unlikely(Size == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Size = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSymmetricStateEncrypt::body(
    Runtime::Instance::MemoryInstance *MemInst, __wasi_symmetric_state_t Handle,
    uint8_t_ptr OutPtr, __wasi_size_t OutLen, const_uint8_t_ptr DataPtr,
    __wasi_size_t DataLen, uint32_t /* Out */ SizePtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *DataMem = MemInst->getPointer<uint8_t const *>(DataPtr, DataLen);
  if (unlikely(DataMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t const> Data{DataMem, DataLen};

  auto *OutMem = MemInst->getPointer<uint8_t *>(OutPtr, OutLen);
  if (unlikely(OutMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t> Out{OutMem, OutLen};

  auto Res = Ctx.symmetricStateEncrypt(Handle, Out, Data);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  if (unlikely(Size == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Size = *Res;
  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSymmetricStateEncryptDetached::body(
    Runtime::Instance::MemoryInstance *MemInst, __wasi_symmetric_state_t Handle,
    uint8_t_ptr OutPtr, __wasi_size_t OutLen, const_uint8_t_ptr DataPtr,
    __wasi_size_t DataLen, uint32_t /* Out */ KeyPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *DataMem = MemInst->getPointer<uint8_t const *>(DataPtr, DataLen);
  if (unlikely(DataMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t const> Data{DataMem, DataLen};

  auto *OutMem = MemInst->getPointer<uint8_t *>(OutPtr, OutLen);
  if (unlikely(OutMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t> Out{OutMem, OutLen};

  auto Res = Ctx.symmetricStateEncryptDetached(Handle, Out, Data);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *Key = MemInst->getPointer<__wasi_symmetric_key_t *>(KeyPtr);
  if (unlikely(Key == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Key = *Res;
  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSymmetricStateDecrypt::body(
    Runtime::Instance::MemoryInstance *MemInst, __wasi_symmetric_state_t Handle,
    uint8_t_ptr OutPtr, __wasi_size_t OutLen, const_uint8_t_ptr DataPtr,
    __wasi_size_t DataLen, uint32_t /* Out */ SizePtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *DataMem = MemInst->getPointer<uint8_t const *>(DataPtr, DataLen);
  if (unlikely(DataMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t const> Data{DataMem, DataLen};

  auto *OutMem = MemInst->getPointer<uint8_t *>(OutPtr, OutLen);
  if (unlikely(OutMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t> Out{OutMem, OutLen};

  auto Res = Ctx.symmetricStateDecrypt(Handle, Out, Data);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  if (unlikely(Size == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Size = *Res;
  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSymmetricStateDecryptDetached::body(
    Runtime::Instance::MemoryInstance *MemInst, __wasi_symmetric_state_t Handle,
    uint8_t_ptr OutPtr, __wasi_size_t OutLen, const_uint8_t_ptr DataPtr,
    __wasi_size_t DataLen, uint8_t_ptr RawTagPtr, __wasi_size_t RawTagLen,
    uint32_t /* Out */ SizePtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *DataMem = MemInst->getPointer<uint8_t const *>(DataPtr, DataLen);
  if (unlikely(DataMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t const> Data{DataMem, DataLen};

  auto *OutMem = MemInst->getPointer<uint8_t *>(OutPtr, OutLen);
  if (unlikely(OutMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t> Out{OutMem, OutLen};

  auto *RawTagMem = MemInst->getPointer<uint8_t *>(RawTagPtr, RawTagLen);
  if (unlikely(RawTagMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t> RawTag{RawTagMem, RawTagLen};

  auto Res = Ctx.symmetricStateDecryptDetached(Handle, Out, Data, RawTag);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  if (unlikely(Size == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Size = *Res;
  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSymmetricStateRatchet::body(Runtime::Instance::MemoryInstance *MemInst,
                                __wasi_symmetric_state_t Handle) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.symmetricStateRatchet(Handle);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSymmetricTagLen::body(Runtime::Instance::MemoryInstance *MemInst,
                          __wasi_symmetric_tag_t SymmetricTag,
                          uint32_t /* Out */ SizePtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.symmetricTagLen(SymmetricTag);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  if (unlikely(Size == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Size = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSymmetricTagPull::body(Runtime::Instance::MemoryInstance *MemInst,
                           __wasi_symmetric_tag_t SymmetricTag,
                           uint8_t_ptr BufPtr, __wasi_size_t BufLen,
                           uint32_t /* Out */ SizePtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *BufMem = MemInst->getPointer<uint8_t *>(BufPtr, BufLen);
  if (unlikely(BufMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t> Buf{BufMem, BufLen};

  auto Res = Ctx.symmetricTagPull(SymmetricTag, Buf);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  if (unlikely(Size == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Size = *Res;
  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSymmetricTagVerify::body(Runtime::Instance::MemoryInstance *MemInst,
                             __wasi_symmetric_tag_t SymmetricTag,
                             uint8_t_ptr RawTagPtr, __wasi_size_t RawTagLen) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *RawTagMem = MemInst->getPointer<uint8_t *>(RawTagPtr, RawTagLen);
  if (unlikely(RawTagMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t> RawTag{RawTagMem, RawTagLen};

  auto Res = Ctx.symmetricTagVerify(SymmetricTag, RawTag);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSymmetricTagClose::body(Runtime::Instance::MemoryInstance *MemInst,
                            __wasi_symmetric_tag_t SymmetricTag) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.symmetricTagClose(SymmetricTag);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}
} // namespace Host
} // namespace WasmEdge
