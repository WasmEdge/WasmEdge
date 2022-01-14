// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/asymmetric_common/func.h"
#include "host/wasi_crypto/util.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace AsymmetricCommon {

Expect<uint32_t>
KeypairGenerate::body(Runtime::Instance::MemoryInstance *MemInst,
                      uint32_t AlgorithmType, const_uint8_t_ptr AlgorithmPtr,
                      __wasi_size_t AlgorithmLen, uint32_t Options,
                      uint8_t_ptr /* Out */ KeypairPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto AlgType = cast<__wasi_algorithm_type_e_t>(AlgorithmType);
  if (unlikely(!AlgType)) {
    return AlgType.error();
  }

  auto *AlgMem = MemInst->getPointer<const char *>(AlgorithmPtr, AlgorithmLen);
  if (unlikely(AlgMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  std::string_view AlgStr{AlgMem, AlgorithmLen};

  auto *OptOptions = MemInst->getPointer<__wasi_opt_options_t *>(Options);
  if (unlikely(OptOptions == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  auto Res = Ctx.keypairGenerate(*AlgType, AlgStr,
                                 parseCUnion<__wasi_options_t>(*OptOptions));
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *Keypair = MemInst->getPointer<__wasi_keypair_t *>(KeypairPtr);
  if (unlikely(Keypair == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Keypair = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
KeypairImport::body(Runtime::Instance::MemoryInstance *MemInst,
                    uint32_t AlgorithmType, const_uint8_t_ptr AlgorithmPtr,
                    __wasi_size_t AlgorithmLen, const_uint8_t_ptr EncodedPtr,
                    __wasi_size_t EncodedLen, uint32_t Encoding,
                    uint8_t_ptr /* Out */ KeypairPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto AlgType = cast<__wasi_algorithm_type_e_t>(AlgorithmType);
  if (unlikely(!AlgType)) {
    return AlgType.error();
  }

  auto *AlgMem = MemInst->getPointer<const char *>(AlgorithmPtr, AlgorithmLen);
  if (unlikely(AlgMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  std::string_view AlgStr{AlgMem, AlgorithmLen};

  auto *EncodedMem = MemInst->getPointer<uint8_t *>(EncodedPtr, EncodedLen);
  if (unlikely(EncodedMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t> Encoded{EncodedMem, EncodedLen};

  auto EncodingEnum = cast<__wasi_keypair_encoding_e_t>(Encoding);
  if (!EncodingEnum) {
    return EncodingEnum.error();
  }

  auto Res = Ctx.keypairImport(*AlgType, AlgStr, Encoded, *EncodingEnum);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *Keypair = MemInst->getPointer<__wasi_keypair_t *>(KeypairPtr);
  if (unlikely(Keypair == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Keypair = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeypairGenerateManaged::body(
    Runtime::Instance::MemoryInstance *MemInst,
    __wasi_secrets_manager_t SecretsManager, uint32_t AlgorithmType,
    const_uint8_t_ptr AlgorithmPtr, __wasi_size_t AlgorithmLen,
    uint8_t_ptr OptOptionsPtr, uint8_t_ptr /* Out */ ResultPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto AlgType = cast<__wasi_algorithm_type_e_t>(AlgorithmType);
  if (unlikely(!AlgType)) {
    return AlgType.error();
  }

  auto *AlgMem = MemInst->getPointer<const char *>(AlgorithmPtr, AlgorithmLen);
  if (unlikely(AlgMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  std::string_view AlgStr{AlgMem, AlgorithmLen};

  auto *OptOptions = MemInst->getPointer<__wasi_opt_options_t *>(OptOptionsPtr);
  if (unlikely(OptOptions == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  auto Res =
      Ctx.keypairGenerateManaged(SecretsManager, *AlgType, AlgStr,
                                 parseCUnion<__wasi_options_t>(*OptOptions));
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *Keypair = MemInst->getPointer<__wasi_keypair_t *>(ResultPtr);
  if (unlikely(Keypair == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  *Keypair = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
KeypairStoreManaged::body(Runtime::Instance::MemoryInstance *MemInst,
                          __wasi_secrets_manager_t SecretsManager,
                          __wasi_keypair_t Kp, uint8_t_ptr KpIdPtr,
                          __wasi_size_t KpIdMaxLen) {

  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  auto Res = Ctx.keypairStoreManaged(SecretsManager, Kp, KpIdPtr, KpIdMaxLen);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
KeypairReplaceManaged::body(Runtime::Instance::MemoryInstance *MemInst,
                            __wasi_secrets_manager_t SecretsManager,
                            __wasi_keypair_t KpOld, __wasi_keypair_t KpNew,
                            uint8_t_ptr /* Out */ VersionPtr) {

  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.keypairReplaceManaged(SecretsManager, KpOld, KpNew);
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

Expect<uint32_t> KeypairId::body(Runtime::Instance::MemoryInstance *MemInst,
                                 __wasi_keypair_t Kp, uint8_t_ptr KpId,
                                 __wasi_size_t KpIdMaxLen,
                                 uint8_t_ptr /* Out */ SizePtr,
                                 uint8_t_ptr /* Out */ VersionPtr) {

  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.keypairId(Kp, KpId, KpIdMaxLen);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto [ResSize, ResVersion] = *Res;

  auto SafeResSize = cast<__wasi_size_t>(ResSize);
  if (unlikely(!SafeResSize)) {
    return SafeResSize.error();
  }

  auto *Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  if (unlikely(Size == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Size = *SafeResSize;

  auto *Version = MemInst->getPointer<__wasi_version_t *>(VersionPtr);
  if (unlikely(Version == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Version = ResVersion;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeypairFromId::body(Runtime::Instance::MemoryInstance *MemInst,
                                     __wasi_secrets_manager_t SecretsManager,
                                     const_uint8_t_ptr KpId,
                                     __wasi_size_t KpIdLen,
                                     __wasi_version_t KpVersion,
                                     uint8_t_ptr /* Out */ KeypairPtr) {

  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.keypairFromId(SecretsManager, KpId, KpIdLen, KpVersion);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *Keypair = MemInst->getPointer<__wasi_version_t *>(KeypairPtr);
  if (unlikely(Keypair == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Keypair = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeypairFromPkAndSk::body(
    Runtime::Instance::MemoryInstance *MemInst, __wasi_publickey_t Publickey,
    __wasi_secretkey_t Secretkey, uint8_t_ptr /* Out */ KeypairPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.keypairFromPkAndSk(Publickey, Secretkey);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *Keypair = MemInst->getPointer<__wasi_keypair_t *>(KeypairPtr);
  if (unlikely(Keypair == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Keypair = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeypairExport::body(Runtime::Instance::MemoryInstance *MemInst,
                                     __wasi_keypair_t Kp, uint32_t Encoding,
                                     uint8_t_ptr /* Out */ ArrayOutputPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto EncodingEnum = cast<__wasi_keypair_encoding_e_t>(Encoding);
  if (unlikely(!EncodingEnum)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.keypairExport(Kp, *EncodingEnum);
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

Expect<uint32_t>
KeypairPublickey::body(Runtime::Instance::MemoryInstance *MemInst,
                       __wasi_keypair_t Kp, uint8_t_ptr PublicKeyPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.keypairPublickey(Kp);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *PublicKey = MemInst->getPointer<__wasi_keypair_t *>(PublicKeyPtr);
  if (unlikely(PublicKey == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *PublicKey = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
KeypairSecretkey::body(Runtime::Instance::MemoryInstance *MemInst,
                       __wasi_keypair_t Kp,
                       uint8_t_ptr /* Out */ SecretKeyPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.keypairSecretkey(Kp);
  if (unlikely(!Res)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *SecretKey = MemInst->getPointer<__wasi_keypair_t *>(SecretKeyPtr);
  if (unlikely(SecretKey == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *SecretKey = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> KeypairClose::body(Runtime::Instance::MemoryInstance *MemInst,
                                    __wasi_keypair_t Kp) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.keypairClose(Kp);
  if (unlikely(!Res)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
PublickeyImport::body(Runtime::Instance::MemoryInstance *MemInst,
                      uint32_t AlgorithmType, const_uint8_t_ptr AlgorithmPtr,
                      __wasi_size_t AlgorithmLen, const_uint8_t_ptr EncodedPtr,
                      __wasi_size_t EncodedLen, uint32_t Encoding,
                      uint8_t_ptr /* Out */ PublickeyPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto AlgType = cast<__wasi_algorithm_type_e_t>(AlgorithmType);
  if (unlikely(!AlgType)) {
    return AlgType.error();
  }

  auto *AlgMem = MemInst->getPointer<const char *>(AlgorithmPtr, AlgorithmLen);
  if (unlikely(AlgMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  std::string_view AlgStr{AlgMem, AlgorithmLen};

  auto *EncodedMem = MemInst->getPointer<uint8_t *>(EncodedPtr, EncodedLen);
  if (unlikely(EncodedMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t> Encoded{EncodedMem, EncodedLen};

  auto EncodingEnum = cast<__wasi_publickey_encoding_e_t>(Encoding);
  if (!EncodingEnum) {
    return EncodingEnum.error();
  }

  auto Res = Ctx.publickeyImport(*AlgType, AlgStr, Encoded, *EncodingEnum);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *PublicKey = MemInst->getPointer<__wasi_publickey_t *>(PublickeyPtr);
  if (unlikely(PublicKey == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *PublicKey = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
PublickeyExport::body(Runtime::Instance::MemoryInstance *MemInst,
                      __wasi_publickey_t Pk, uint32_t Encoding,
                      uint8_t_ptr /* Out */ ArrayOutputPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto EncodingEnum = cast<__wasi_publickey_encoding_e_t>(Encoding);
  if (unlikely(!EncodingEnum)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.publickeyExport(Pk, *EncodingEnum);
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

Expect<uint32_t>
PublickeyVerify::body(Runtime::Instance::MemoryInstance *MemInst,
                      __wasi_publickey_t Pk) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.publickeyVerify(Pk);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
PublickeyFromSecretkey::body(Runtime::Instance::MemoryInstance *MemInst,
                             __wasi_secretkey_t Sk,
                             uint8_t_ptr /* Out */ PublickeyPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.publickeyFromSecretkey(Sk);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *Publickey = MemInst->getPointer<__wasi_publickey_t *>(PublickeyPtr);
  if (unlikely(Publickey == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Publickey = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
PublickeyClose::body(Runtime::Instance::MemoryInstance *MemInst,
                     __wasi_publickey_t Pk) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.publickeyClose(Pk);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}
Expect<uint32_t>
SecretkeyImport::body(Runtime::Instance::MemoryInstance *MemInst,
                      uint32_t AlgorithmType, const_uint8_t_ptr AlgorithmPtr,
                      __wasi_size_t AlgorithmLen, const_uint8_t_ptr EncodedPtr,
                      __wasi_size_t EncodedLen, uint32_t Encoding,
                      uint8_t_ptr SecretkeyPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto AlgType = cast<__wasi_algorithm_type_e_t>(AlgorithmType);
  if (unlikely(!AlgType)) {
    return AlgType.error();
  }

  auto *AlgMem = MemInst->getPointer<const char *>(AlgorithmPtr, AlgorithmLen);
  if (unlikely(AlgMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  std::string_view AlgStr{AlgMem, AlgorithmLen};

  auto *EncodedMem = MemInst->getPointer<uint8_t *>(EncodedPtr, EncodedLen);
  if (unlikely(EncodedMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t> Encoded{EncodedMem, EncodedLen};

  auto EncodingEnum = cast<__wasi_secretkey_encoding_e_t>(Encoding);
  if (!EncodingEnum) {
    return EncodingEnum.error();
  }

  auto Res = Ctx.secretkeyImport(*AlgType, AlgStr, Encoded, *EncodingEnum);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *Secretkey = MemInst->getPointer<__wasi_secretkey_t *>(SecretkeyPtr);
  if (unlikely(Secretkey == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Secretkey = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
SecretkeyExport::body(Runtime::Instance::MemoryInstance *MemInst,
                      __wasi_secretkey_t Sk, uint32_t Encoding,
                      uint8_t_ptr /* Out */ ArrayOutputPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto EncodingEnum = cast<__wasi_secretkey_encoding_e_t>(Encoding);
  if (unlikely(!EncodingEnum)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.secretkeyExport(Sk, *EncodingEnum);
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

Expect<uint32_t>
SecretkeyClose::body(Runtime::Instance::MemoryInstance *MemInst,
                     __wasi_secretkey_t Sk) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.secretkeyClose(Sk);
  if (unlikely(!Res)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}
} // namespace AsymmetricCommon
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
