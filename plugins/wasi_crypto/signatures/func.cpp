// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "signatures/func.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

Expect<uint32_t> Export::body(const Runtime::CallingFrame &Frame,
                              int32_t SigHandle, uint32_t Encoding,
                              uint32_t /* Out */ ArrayOutputHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
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

Expect<uint32_t> Import::body(const Runtime::CallingFrame &Frame,
                              uint32_t AlgPtr, uint32_t AlgLen,
                              uint32_t EncodedPtr, uint32_t EncodedLen,
                              uint32_t Encoding,
                              uint32_t /* Out */ SigHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiAlgLen = AlgLen;
  const auto Alg = MemInst->getStringView(AlgPtr, WasiAlgLen);
  checkRangeExist(Alg, WasiAlgLen);

  Algorithm WasiAlg;
  if (auto Res = tryFrom<Algorithm>(Alg); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiAlg = *Res;
  }

  const __wasi_size_t WasiEncodedLen = EncodedLen;
  const auto Encoded =
      MemInst->getSpan<const uint8_t>(EncodedPtr, WasiEncodedLen);
  checkRangeExist(Encoded, WasiEncodedLen);

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

  if (auto Res = Ctx.signatureImport(WasiAlg, Encoded, WasiEncoding);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *SigHandle = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateOpen::body(const Runtime::CallingFrame &Frame,
                                 int32_t KpHandle,
                                 uint32_t /* Out */ SigStatePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
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

Expect<uint32_t> StateUpdate::body(const Runtime::CallingFrame &Frame,
                                   int32_t SigStateHandle, uint32_t InputPtr,
                                   uint32_t InputSize) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiInputSize = InputSize;
  const auto Input = MemInst->getSpan<const uint8_t>(InputPtr, WasiInputSize);
  checkRangeExist(Input, WasiInputSize);

  if (auto Res = Ctx.signatureStateUpdate(SigStateHandle, Input);
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> StateSign::body(const Runtime::CallingFrame &Frame,
                                 int32_t SigStateHandle,
                                 uint32_t /* Out */ ArrayOutputHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
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

Expect<uint32_t> StateClose::body(const Runtime::CallingFrame &Frame,
                                  int32_t SigStateHandle) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  if (auto Res = Ctx.signatureStateClose(SigStateHandle); unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
VerificationStateOpen::body(const Runtime::CallingFrame &Frame,
                            int32_t SigPkHandle,
                            uint32_t /* Out */ VerificationStateHandlePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
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
VerificationStateUpdate::body(const Runtime::CallingFrame &Frame,
                              int32_t SigStateHandle, uint32_t InputPtr,
                              uint32_t InputSize) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  checkExist(MemInst);

  const __wasi_size_t WasiInputSize = InputSize;
  const auto Input = MemInst->getSpan<const uint8_t>(InputPtr, WasiInputSize);
  checkRangeExist(Input, WasiInputSize);

  if (auto Res = Ctx.signatureVerificationStateUpdate(SigStateHandle, Input);
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> VerificationStateVerify::body(const Runtime::CallingFrame &,
                                               int32_t VerificationStateHandle,
                                               int32_t SigHandle) {
  if (auto Res = Ctx.signatureVerificationStateVerify(VerificationStateHandle,
                                                      SigHandle);
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> VerificationStateClose::body(const Runtime::CallingFrame &,
                                              int32_t VerificationStateHandle) {
  if (auto Res = Ctx.signatureVerificationStateClose(VerificationStateHandle);
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> Close::body(const Runtime::CallingFrame &, int32_t SigHandle) {
  if (auto Res = Ctx.signatureClose(SigHandle); unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

} // namespace Signatures
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
