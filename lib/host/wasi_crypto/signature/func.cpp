// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/signature/func.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/signature/alg.h"
#include "host/wasi_crypto/util.h"
#include "runtime/instance/memory.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {

Expect<uint32_t> Export::body(Runtime::Instance::MemoryInstance *MemInst,
                              __wasi_signature_t Signature, uint16_t Encoding,
                              uint32_t ArrayOutputPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto EncodingType = cast<__wasi_signature_encoding_e_t>(Encoding);
  if (unlikely(!EncodingType)) {
    return EncodingType.error();
  }

  auto Res = Ctx.signatureExport(Signature, *EncodingType);
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

Expect<uint32_t> Import::body(Runtime::Instance::MemoryInstance *MemInst,
                              const_uint8_t_ptr AlgorithmPtr,
                              __wasi_size_t AlgorithmLen,
                              const_uint8_t_ptr EncodedPtr,
                              __wasi_size_t EncodedLen, uint16_t Encoding,
                              uint32_t /* Out */ SignaturePtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *AlgMem = MemInst->getPointer<const char *>(AlgorithmPtr, AlgorithmLen);
  if (unlikely(AlgMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  std::string_view Alg{AlgMem, AlgorithmLen};
  auto EnumAlg = tryFrom<SignatureAlgorithm>(Alg);
  if (!EnumAlg) {
    return EnumAlg.error();
  }

  auto *EncodedMem = MemInst->getPointer<uint8_t *>(EncodedPtr, EncodedLen);
  if (unlikely(EncodedMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t const> Encoded{EncodedMem, EncodedLen};

  auto EncodingType = cast<__wasi_signature_encoding_e_t>(Encoding);
  if (unlikely(!EncodingType)) {
    return EncodingType.error();
  }

  auto Res = Ctx.signatureImport(*EnumAlg, Encoded, *EncodingType);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *Signature = MemInst->getPointer<__wasi_signature_t *>(SignaturePtr);
  if (unlikely(Signature == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *Signature = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateOpen::body(Runtime::Instance::MemoryInstance *MemInst,
                                 __wasi_signature_keypair_t Kp,
                                 uint32_t /* Out */ SignatureStatePtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.signatureStateOpen(Kp);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *SignatureState =
      MemInst->getPointer<__wasi_signature_state_t *>(SignatureStatePtr);
  if (unlikely(SignatureState == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *SignatureState = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateUpdate::body(Runtime::Instance::MemoryInstance *MemInst,
                                   __wasi_signature_state_t State,
                                   const_uint8_t_ptr InputPtr,
                                   __wasi_size_t InputSize) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *InputMem = MemInst->getPointer<uint8_t *>(InputPtr, InputSize);
  if (unlikely(InputMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t const> Input{InputMem, InputSize};

  auto Res = Ctx.signatureStateUpdate(State, Input);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
StateSign::body(Runtime::Instance::MemoryInstance *MemInst,
                __wasi_signature_state_t State,
                __wasi_array_output_t /* Out */ ArrayOutputPtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.signatureStateSign(State);
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

Expect<uint32_t> StateClose::body(Runtime::Instance::MemoryInstance *MemInst,
                                  __wasi_signature_state_t State) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.signatureStateClose(State);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
VerificationStateOpen::body(Runtime::Instance::MemoryInstance *MemInst,
                            __wasi_signature_publickey_t Pk,
                            uint32_t /* Out */ SignatureVerificationStatePtr) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.signatureVerificationStateOpen(Pk);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *SignatureVerificationState =
      MemInst->getPointer<__wasi_signature_state_t *>(
          SignatureVerificationStatePtr);
  if (unlikely(SignatureVerificationState == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *SignatureVerificationState = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
VerificationStateUpdate::body(Runtime::Instance::MemoryInstance *MemInst,
                              __wasi_signature_verification_state_t State,
                              const_uint8_t_ptr InputPtr,
                              __wasi_size_t InputSize) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *InputMem = MemInst->getPointer<uint8_t *>(InputPtr, InputSize);
  if (unlikely(InputMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t const> Input{InputMem, InputSize};

  auto Res = Ctx.signatureVerificationStateUpdate(State, Input);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
VerificationStateVerify::body(Runtime::Instance::MemoryInstance *MemInst,
                              __wasi_signature_verification_state_t State,
                              __wasi_signature_t Signature) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.signatureVerificationStateVerify(State, Signature);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
VerificationStateClose::body(Runtime::Instance::MemoryInstance *MemInst,
                             __wasi_signature_verification_state_t State) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.signatureVerificationStateClose(State);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> Close::body(Runtime::Instance::MemoryInstance *MemInst,
                             __wasi_signature_t Signature) {
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.signatureClose(Signature);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
