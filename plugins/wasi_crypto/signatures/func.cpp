// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "signatures/func.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

Expect<uint32_t> Export::body(Runtime::Instance::MemoryInstance *MemInst,
                              int32_t SigHandle, uint32_t Encoding,
                              uint32_t /* Out */ ArrayOutputHandlePtr) {
  checkExist(MemInst);

  __wasi_signature_encoding_e_t WasiEncoding;
  if (auto Res = cast<__wasi_signature_encoding_e_t>(Encoding);
      unlikely(!Res)) {
    return Res.error();
  } else {
    WasiEncoding = *Res;
  }

  auto *const ArrayOutput =
      MemInst->getPointer<__wasi_array_output_t *>(ArrayOutputHandlePtr);
  checkExist(ArrayOutput);

  if (auto Res = Ctx.signatureExport(SigHandle, WasiEncoding); unlikely(!Res)) {
    return Res.error();
  } else {
    *ArrayOutput = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> Import::body(Runtime::Instance::MemoryInstance *MemInst,
                              uint32_t AlgPtr, uint32_t AlgLen,
                              uint32_t EncodedPtr, uint32_t EncodedLen,
                              uint32_t Encoding,
                              uint32_t /* Out */ SigHandlePtr) {
  checkExist(MemInst);

  const __wasi_size_t WasiAlgLen = AlgLen;
  auto *const Alg = MemInst->getPointer<const char *>(AlgPtr, WasiAlgLen);
  checkExist(Alg);

  Algorithm WasiAlg;
  if (auto Res = tryFrom<Algorithm>({Alg, AlgLen}); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiAlg = *Res;
  }

  const __wasi_size_t WasiEncodedLen = EncodedLen;
  auto *const Encoded =
      MemInst->getPointer<const uint8_t *>(EncodedPtr, WasiEncodedLen);
  checkExist(Encoded);

  __wasi_signature_encoding_e_t WasiEncoding;
  if (auto Res = cast<__wasi_signature_encoding_e_t>(Encoding);
      unlikely(!Res)) {
    return Res.error();
  } else {
    WasiEncoding = *Res;
  }

  auto *const SigHandle =
      MemInst->getPointer<__wasi_signature_t *>(SigHandlePtr);
  checkExist(SigHandle);

  if (auto Res =
          Ctx.signatureImport(WasiAlg, {Encoded, WasiEncodedLen}, WasiEncoding);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *SigHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateOpen::body(Runtime::Instance::MemoryInstance *MemInst,
                                 int32_t KpHandle,
                                 uint32_t /* Out */ SigStatePtr) {
  checkExist(MemInst);

  auto *const SigState =
      MemInst->getPointer<__wasi_signature_state_t *>(SigStatePtr);
  checkExist(SigState);

  if (auto Res = Ctx.signatureStateOpen(KpHandle); unlikely(!Res)) {
    return Res.error();
  } else {
    *SigState = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateUpdate::body(Runtime::Instance::MemoryInstance *MemInst,
                                   int32_t SigStateHandle, uint32_t InputPtr,
                                   uint32_t InputSize) {
  checkExist(MemInst);

  const __wasi_size_t WasiInputSize = InputSize;
  auto *const Input =
      MemInst->getPointer<const uint8_t *>(InputPtr, WasiInputSize);
  checkExist(Input);

  if (auto Res =
          Ctx.signatureStateUpdate(SigStateHandle, {Input, WasiInputSize});
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateSign::body(Runtime::Instance::MemoryInstance *MemInst,
                                 int32_t SigStateHandle,
                                 uint32_t /* Out */ ArrayOutputHandlePtr) {
  checkExist(MemInst);

  auto *const ArrayOutputHandle =
      MemInst->getPointer<__wasi_array_output_t *>(ArrayOutputHandlePtr);
  checkExist(ArrayOutputHandle);

  if (auto Res = Ctx.signatureStateSign(SigStateHandle); unlikely(!Res)) {
    return Res.error();
  } else {
    *ArrayOutputHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateClose::body(Runtime::Instance::MemoryInstance *MemInst,
                                  int32_t SigStateHandle) {
  checkExist(MemInst);

  if (auto Res = Ctx.signatureStateClose(SigStateHandle); unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
VerificationStateOpen::body(Runtime::Instance::MemoryInstance *MemInst,
                            int32_t SigPkHandle,
                            uint32_t /* Out */ VerificationStateHandlePtr) {
  checkExist(MemInst);

  auto *const VerificationStateHandle =
      MemInst->getPointer<__wasi_signature_state_t *>(
          VerificationStateHandlePtr);
  checkExist(VerificationStateHandle);

  if (auto Res = Ctx.signatureVerificationStateOpen(SigPkHandle);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *VerificationStateHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
VerificationStateUpdate::body(Runtime::Instance::MemoryInstance *MemInst,
                              int32_t SigStateHandle, uint32_t InputPtr,
                              uint32_t InputSize) {
  checkExist(MemInst);

  const __wasi_size_t WasiInputSize = InputSize;
  auto *const Input = MemInst->getPointer<const uint8_t *>(InputPtr, InputSize);
  checkExist(Input);

  if (auto Res = Ctx.signatureVerificationStateUpdate(SigStateHandle,
                                                      {Input, WasiInputSize});
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
VerificationStateVerify::body(Runtime::Instance::MemoryInstance *,
                              int32_t VerificationStateHandle,
                              int32_t SigHandle) {
  if (auto Res = Ctx.signatureVerificationStateVerify(VerificationStateHandle,
                                                      SigHandle);
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
VerificationStateClose::body(Runtime::Instance::MemoryInstance *,
                             int32_t VerificationStateHandle) {
  if (auto Res = Ctx.signatureVerificationStateClose(VerificationStateHandle);
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> Close::body(Runtime::Instance::MemoryInstance *,
                             int32_t SigHandle) {
  if (auto Res = Ctx.signatureClose(SigHandle); unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

} // namespace Signatures
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
