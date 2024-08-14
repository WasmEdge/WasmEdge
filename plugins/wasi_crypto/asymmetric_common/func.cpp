// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "asymmetric_common/func.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace AsymmetricCommon {

Expect<uint32_t> KeypairGenerate::body(const Runtime::CallingFrame &Frame,
                                       uint32_t AlgType, uint32_t AlgPtr,
                                       uint32_t AlgLen,
                                       uint32_t OptOptionsHandlePtr,
                                       uint32_t /* Out */ KpHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiAlgLen = AlgLen;
  const auto Alg = MemInst->getStringView(AlgPtr, WasiAlgLen);
  checkRangeExist(Alg, WasiAlgLen);

  AsymmetricCommon::Algorithm WasiAlg;
  if (auto Res = cast<__wasi_algorithm_type_e_t>(AlgType).and_then(
          [Alg](auto WasiAlgType) { return tryFrom(WasiAlgType, Alg); });
      unlikely(!Res)) {
    return Res.error();
  } else {
    WasiAlg = *Res;
  }

  auto *const OptOptionsHandle =
      MemInst->getPointer<const __wasi_opt_options_t *>(OptOptionsHandlePtr);
  checkExist(OptOptionsHandle);

  auto *const KpHandle = MemInst->getPointer<__wasi_keypair_t *>(KpHandlePtr);
  checkExist(KpHandle);

  if (auto Res = Ctx.keypairGenerate(WasiAlg, *OptOptionsHandle);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *KpHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeypairImport::body(const Runtime::CallingFrame &Frame,
                                     uint32_t AlgType, uint32_t AlgPtr,
                                     uint32_t AlgLen, uint32_t EncodedPtr,
                                     uint32_t EncodedLen, uint32_t Encoding,
                                     uint32_t /* Out */ KpHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiAlgLen = AlgLen;
  const auto Alg = MemInst->getStringView(AlgPtr, WasiAlgLen);
  checkRangeExist(Alg, WasiAlgLen);

  AsymmetricCommon::Algorithm WasiAlg;
  if (auto Res = cast<__wasi_algorithm_type_e_t>(AlgType).and_then(
          [Alg](auto WasiAlgType) { return tryFrom(WasiAlgType, Alg); });
      unlikely(!Res)) {
    return Res.error();
  } else {
    WasiAlg = *Res;
  }

  const __wasi_size_t WasiEncodedLen = EncodedLen;
  const auto Encoded =
      MemInst->getSpan<const uint8_t>(EncodedPtr, WasiEncodedLen);
  checkRangeExist(Encoded, WasiEncodedLen);

  const auto WasiEncoding = cast<__wasi_keypair_encoding_e_t>(Encoding);
  checkExist(WasiEncoding);

  auto *const KpHandle = MemInst->getPointer<__wasi_keypair_t *>(KpHandlePtr);
  checkExist(KpHandle);

  if (auto Res = Ctx.keypairImport(WasiAlg, Encoded, *WasiEncoding);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *KpHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeypairGenerateManaged::body(
    const Runtime::CallingFrame &Frame, int32_t SecretsManagerHandle,
    uint32_t AlgType, uint32_t AlgPtr, uint32_t AlgLen,
    uint32_t OptOptionsHandlePtr, uint32_t KpHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiAlgLen = AlgLen;
  const auto Alg = MemInst->getStringView(AlgPtr, WasiAlgLen);
  checkRangeExist(Alg, WasiAlgLen);

  AsymmetricCommon::Algorithm WasiAlg;
  if (auto Res = cast<__wasi_algorithm_type_e_t>(AlgType).and_then(
          [Alg](auto WasiAlgType) { return tryFrom(WasiAlgType, Alg); });
      unlikely(!Res)) {
    return Res.error();
  } else {
    WasiAlg = *Res;
  }

  auto *const OptOptionsHandle =
      MemInst->getPointer<const __wasi_opt_options_t *>(OptOptionsHandlePtr);
  checkExist(OptOptionsHandle);

  auto *const KpHandle = MemInst->getPointer<__wasi_keypair_t *>(KpHandlePtr);
  checkExist(KpHandle);

  if (auto Res = Ctx.keypairGenerateManaged(SecretsManagerHandle, WasiAlg,
                                            *OptOptionsHandle);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *KpHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeypairStoreManaged::body(const Runtime::CallingFrame &Frame,
                                           int32_t SecretsManagerHandle,
                                           int32_t KpHandle, uint32_t KpIdPtr,
                                           uint32_t KpIdMaxLen) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiKpIdMaxLen = KpIdMaxLen;
  const auto KpId = MemInst->getSpan<uint8_t>(KpIdPtr, WasiKpIdMaxLen);
  checkRangeExist(KpId, WasiKpIdMaxLen);

  if (auto Res = Ctx.keypairStoreManaged(SecretsManagerHandle, KpHandle, KpId);
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeypairReplaceManaged::body(const Runtime::CallingFrame &Frame,
                                             int32_t SecretsManagerHandle,
                                             int32_t OldKpHandle,
                                             int32_t NewKpHandle,
                                             uint32_t /* Out */ KpVersionPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  auto *const KpVersion = MemInst->getPointer<__wasi_version_t *>(KpVersionPtr);
  checkExist(KpVersion);

  if (auto Res = Ctx.keypairReplaceManaged(SecretsManagerHandle, OldKpHandle,
                                           NewKpHandle);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *KpVersion = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeypairId::body(const Runtime::CallingFrame &Frame,
                                 int32_t KpHandle, uint32_t KpIdPtr,
                                 uint32_t KpIdMaxLen,
                                 uint32_t /* Out */ SizePtr,
                                 uint32_t /* Out */ KpVersionPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiKpIdMaxLen = KpIdMaxLen;
  const auto KpId = MemInst->getSpan<uint8_t>(KpIdPtr, WasiKpIdMaxLen);
  checkRangeExist(KpId, WasiKpIdMaxLen);

  auto *const Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  checkExist(Size);

  auto *const Version = MemInst->getPointer<__wasi_version_t *>(KpVersionPtr);
  checkExist(Version);

  if (auto Res = Ctx.keypairId(KpHandle, KpId); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [ResSize, ResVersion] = *Res;

    auto SafeResSize = toWasiSize(ResSize);
    if (unlikely(!SafeResSize)) {
      return SafeResSize.error();
    }

    *Size = *SafeResSize;

    *Version = ResVersion;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeypairFromId::body(const Runtime::CallingFrame &Frame,
                                     int32_t SecretsManagerHandle,
                                     uint32_t KpIdPtr, uint32_t KpIdLen,
                                     uint64_t KpVersion,
                                     uint32_t /* Out */ KpHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiKpIdLen = KpIdLen;
  const auto KpId = MemInst->getSpan<uint8_t>(KpIdPtr, WasiKpIdLen);
  checkRangeExist(KpId, WasiKpIdLen);

  auto *const KpHandle = MemInst->getPointer<__wasi_keypair_t *>(KpHandlePtr);
  checkExist(KpHandle);

  if (auto Res = Ctx.keypairFromId(SecretsManagerHandle, KpId, KpVersion);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *KpHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeypairFromPkAndSk::body(const Runtime::CallingFrame &Frame,
                                          int32_t PkHandle, int32_t SkHandle,
                                          uint32_t /* Out */ KpHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  auto *const KpHandle = MemInst->getPointer<__wasi_keypair_t *>(KpHandlePtr);
  checkExist(KpHandle);

  if (auto Res = Ctx.keypairFromPkAndSk(PkHandle, SkHandle); unlikely(!Res)) {
    return Res.error();
  } else {
    *KpHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeypairExport::body(const Runtime::CallingFrame &Frame,
                                     int32_t KpHandle, uint32_t KpEncoding,
                                     uint32_t /* Out */ ArrayOutputHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  __wasi_keypair_encoding_e_t WasiKpEncoding;
  if (auto Res = cast<__wasi_keypair_encoding_e_t>(KpEncoding);
      unlikely(!Res)) {
    return Res.error();
  } else {
    WasiKpEncoding = *Res;
  }

  auto *const ArrayOutputHandle =
      MemInst->getPointer<__wasi_array_output_t *>(ArrayOutputHandlePtr);
  checkExist(ArrayOutputHandle);

  if (auto Res = Ctx.keypairExport(KpHandle, WasiKpEncoding); unlikely(!Res)) {
    return Res.error();
  } else {
    *ArrayOutputHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeypairPublickey::body(const Runtime::CallingFrame &Frame,
                                        int32_t KpHandle,
                                        uint32_t /* Out */ PkHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  auto *const PkHandle = MemInst->getPointer<__wasi_keypair_t *>(PkHandlePtr);
  checkExist(PkHandle);

  if (auto Res = Ctx.keypairPublickey(KpHandle); unlikely(!Res)) {
    return Res.error();
  } else {
    *PkHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeypairSecretkey::body(const Runtime::CallingFrame &Frame,
                                        int32_t KpHandle,
                                        uint32_t /* Out */ SkHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  auto *const SkHandle = MemInst->getPointer<__wasi_keypair_t *>(SkHandlePtr);
  checkExist(SkHandle);

  if (auto Res = Ctx.keypairSecretkey(KpHandle); unlikely(!Res)) {
    return Res.error();
  } else {
    *SkHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeypairClose::body(const Runtime::CallingFrame &,
                                    int32_t KpHandle) {
  if (auto Res = Ctx.keypairClose(KpHandle); unlikely(!Res)) {
    return __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> PublickeyImport::body(const Runtime::CallingFrame &Frame,
                                       uint32_t AlgType, uint32_t AlgPtr,
                                       uint32_t AlgLen, uint32_t EncodedPtr,
                                       uint32_t EncodedLen, uint32_t Encoding,
                                       uint32_t /* Out */ PkHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiAlgLen = AlgLen;
  const auto Alg = MemInst->getStringView(AlgPtr, WasiAlgLen);
  checkRangeExist(Alg, WasiAlgLen);

  AsymmetricCommon::Algorithm WasiAlg;
  if (auto Res = cast<__wasi_algorithm_type_e_t>(AlgType).and_then(
          [Alg](auto WasiAlgType) { return tryFrom(WasiAlgType, Alg); });
      unlikely(!Res)) {
    return Res.error();
  } else {
    WasiAlg = *Res;
  }

  const __wasi_size_t WasiEncodedLen = EncodedLen;
  const auto Encoded =
      MemInst->getSpan<const uint8_t>(EncodedPtr, WasiEncodedLen);
  checkRangeExist(Encoded, WasiEncodedLen);

  __wasi_publickey_encoding_e_t WasiPkEncoding;
  if (auto Res = cast<__wasi_publickey_encoding_e_t>(Encoding); !Res) {
    return Res.error();
  } else {
    WasiPkEncoding = *Res;
  }

  auto *const PkHandle = MemInst->getPointer<__wasi_publickey_t *>(PkHandlePtr);
  checkExist(PkHandle);

  if (auto Res = Ctx.publickeyImport(WasiAlg, Encoded, WasiPkEncoding);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *PkHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
PublickeyExport::body(const Runtime::CallingFrame &Frame, int32_t PkHandle,
                      uint32_t PkEncoding,
                      uint32_t /* Out */ ArrayOutputHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  __wasi_publickey_encoding_e_t WasiPkEncoding;
  if (auto Res = cast<__wasi_publickey_encoding_e_t>(PkEncoding); !Res) {
    return Res.error();
  } else {
    WasiPkEncoding = *Res;
  }

  auto *const ArrayOutputHandle =
      MemInst->getPointer<__wasi_array_output_t *>(ArrayOutputHandlePtr);
  checkExist(ArrayOutputHandle);

  if (auto Res = Ctx.publickeyExport(PkHandle, WasiPkEncoding);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *ArrayOutputHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> PublickeyVerify::body(const Runtime::CallingFrame &,
                                       int32_t PkHandle) {
  if (auto Res = Ctx.publickeyVerify(PkHandle); unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
PublickeyFromSecretkey::body(const Runtime::CallingFrame &Frame,
                             int32_t SkHandle, uint32_t /* Out */ PkHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  auto *const PkHandle = MemInst->getPointer<__wasi_publickey_t *>(PkHandlePtr);
  checkExist(PkHandle);

  if (auto Res = Ctx.publickeyFromSecretkey(SkHandle); unlikely(!Res)) {
    return Res.error();
  } else {
    *PkHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> PublickeyClose::body(const Runtime::CallingFrame &,
                                      int32_t PkHandle) {
  if (auto Res = Ctx.publickeyClose(PkHandle); unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> SecretkeyImport::body(const Runtime::CallingFrame &Frame,
                                       uint32_t AlgType, uint32_t AlgPtr,
                                       uint32_t AlgLen, uint32_t EncodedPtr,
                                       uint32_t EncodedLen, uint32_t Encoding,
                                       uint32_t /* Out */ SkHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiAlgLen = AlgLen;
  const auto Alg = MemInst->getStringView(AlgPtr, WasiAlgLen);
  checkRangeExist(Alg, WasiAlgLen);

  AsymmetricCommon::Algorithm WasiAlg;
  if (auto Res = cast<__wasi_algorithm_type_e_t>(AlgType).and_then(
          [Alg](auto WasiAlgType) { return tryFrom(WasiAlgType, Alg); });
      unlikely(!Res)) {
    return Res.error();
  } else {
    WasiAlg = *Res;
  }

  const __wasi_size_t WasiEncodedLen = EncodedLen;
  const auto Encoded =
      MemInst->getSpan<const uint8_t>(EncodedPtr, WasiEncodedLen);
  checkRangeExist(Encoded, WasiEncodedLen);

  auto WasiEncoding = cast<__wasi_secretkey_encoding_e_t>(Encoding);
  if (!WasiEncoding) {
    return WasiEncoding.error();
  }

  auto *const SkHandle = MemInst->getPointer<__wasi_secretkey_t *>(SkHandlePtr);
  checkExist(SkHandle);

  if (auto Res = Ctx.secretkeyImport(WasiAlg, Encoded, *WasiEncoding);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *SkHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
SecretkeyExport::body(const Runtime::CallingFrame &Frame, int32_t SkHandle,
                      uint32_t SkEncoding,
                      uint32_t /* Out */ ArrayOutputHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  __wasi_secretkey_encoding_e_t WasiSkEncoding;
  if (auto Res = cast<__wasi_secretkey_encoding_e_t>(SkEncoding); !Res) {
    return Res.error();
  } else {
    WasiSkEncoding = *Res;
  }

  auto *const ArrayOutputHandle =
      MemInst->getPointer<__wasi_array_output_t *>(ArrayOutputHandlePtr);
  checkExist(ArrayOutputHandle);

  if (auto Res = Ctx.secretkeyExport(SkHandle, WasiSkEncoding);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *ArrayOutputHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> SecretkeyClose::body(const Runtime::CallingFrame &,
                                      int32_t Sk) {
  if (auto Res = Ctx.secretkeyClose(Sk); unlikely(!Res)) {
    return __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

} // namespace AsymmetricCommon
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
