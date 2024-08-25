// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "tensorflowlite_func.h"

#include "common/span.h"
#include "common/spdlog.h"

#include "tensorflow/lite/c/c_api.h"

#include <string>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeTensorflowLite {

namespace {

#define MEMINST_CHECK(Out, CallFrame, Index)                                   \
  auto *Out = CallFrame.getMemoryByIndex(Index);                               \
  if (unlikely(Out == nullptr)) {                                              \
    spdlog::error("[WasmEdge-Tensorflow-Lite] Memory instance not found."sv);  \
    return static_cast<uint32_t>(ErrNo::MissingMemory);                        \
  }

#define SESSION_CHECK(Out, SessionID, Message, ErrNo)                          \
  auto *Out = Env.getContext(SessionID);                                       \
  if (unlikely(Out == nullptr)) {                                              \
    spdlog::error("[WasmEdge-Tensorflow-Lite] "sv Message);                    \
    return static_cast<uint32_t>(ErrNo);                                       \
  }

#define MEM_SPAN_CHECK(OutSpan, MemInst, Type, BufPtr, BufLen, Message)        \
  auto OutSpan = MemInst->getSpan<Type>(BufPtr, BufLen);                       \
  if (unlikely(OutSpan.size() != BufLen)) {                                    \
    spdlog::error("[WasmEdge-Tensorflow-Lite] "sv Message);                    \
    return static_cast<uint32_t>(ErrNo::MissingMemory);                        \
  }

#define MEM_SV_CHECK(OutSV, MemInst, BufPtr, BufLen, Message)                  \
  auto OutSV = MemInst->getStringView(BufPtr, BufLen);                         \
  if (unlikely(OutSV.size() != BufLen)) {                                      \
    spdlog::error("[WasmEdge-Tensorflow-Lite] "sv Message);                    \
    return static_cast<uint32_t>(ErrNo::MissingMemory);                        \
  }

#define MEM_PTR_CHECK(OutPtr, MemInst, Type, Offset, Message)                  \
  Type *OutPtr = MemInst->getPointer<Type *>(Offset);                          \
  if (unlikely(OutPtr == nullptr)) {                                           \
    spdlog::error("[WasmEdge-Tensorflow-Lite] "sv Message);                    \
    return static_cast<uint32_t>(ErrNo::MissingMemory);                        \
  }

} // namespace

Expect<uint32_t> CreateSession::body(const Runtime::CallingFrame &Frame,
                                     uint32_t ModBufPtr, uint32_t ModBufLen,
                                     uint32_t SessionIdPtr) {
  // Check memory instance from module.
  MEMINST_CHECK(MemInst, Frame, 0)

  // Check the input model buffer.
  MEM_SPAN_CHECK(ModBufSpan, MemInst, char, ModBufPtr, ModBufLen,
                 "Failed when accessing the input model buffer memory."sv)

  // Check the return value: SessionIdPtr should be valid.
  MEM_PTR_CHECK(SessionId, MemInst, uint32_t, SessionIdPtr,
                "Failed when accessing the return SessionID memory."sv)

  // Create context and import graph.
  uint32_t NewID = Env.newContext();
  SESSION_CHECK(Cxt, NewID, "Failed when allocating resources."sv,
                ErrNo::MissingMemory)

  auto *Model = TfLiteModelCreate(ModBufSpan.data(), ModBufLen);
  if (unlikely(Model == nullptr)) {
    spdlog::error("[WasmEdge-Tensorflow-Lite] Cannot import TFLite model."sv);
    Env.deleteContext(NewID);
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }
  auto *Ops = TfLiteInterpreterOptionsCreate();
  if (unlikely(Ops == nullptr)) {
    spdlog::error(
        "[WasmEdge-Tensorflow-Lite] Cannot create TFLite interpreter options."sv);
    Env.deleteContext(NewID);
    TfLiteModelDelete(Model);
    return static_cast<uint32_t>(ErrNo::Busy);
  }
  TfLiteInterpreterOptionsSetNumThreads(Ops, 2);
  Cxt->Interp = TfLiteInterpreterCreate(Model, Ops);
  TfLiteInterpreterOptionsDelete(Ops);
  TfLiteModelDelete(Model);
  if (unlikely(Cxt->Interp == nullptr)) {
    spdlog::error(
        "[WasmEdge-Tensorflow-Lite] Cannot create TFLite interpreter."sv);
    Env.deleteContext(NewID);
    return static_cast<uint32_t>(ErrNo::Busy);
  }
  TfLiteStatus Status = TfLiteInterpreterAllocateTensors(Cxt->Interp);
  if (unlikely(Status != TfLiteStatus::kTfLiteOk)) {
    spdlog::error("[WasmEdge-Tensorflow-Lite] Cannot allocate tensors."sv);
    Env.deleteContext(NewID);
    return static_cast<uint32_t>(ErrNo::Busy);
  }

  *SessionId = NewID;
  return static_cast<uint32_t>(ErrNo::Success);
}

Expect<uint32_t> DeleteSession::body(const Runtime::CallingFrame &,
                                     uint32_t SessionId) {
  Env.deleteContext(SessionId);
  return static_cast<uint32_t>(ErrNo::Success);
}

Expect<uint32_t> RunSession::body(const Runtime::CallingFrame &,
                                  uint32_t SessionId) {
  // Get context from ID.
  SESSION_CHECK(Cxt, SessionId, "Invalid session ID."sv, ErrNo::InvalidArgument)

  // Run session
  TfLiteStatus Stat = TfLiteInterpreterInvoke(Cxt->Interp);
  if (unlikely(Stat != TfLiteStatus::kTfLiteOk)) {
    spdlog::error("[WasmEdge-Tensorflow-Lite] Invocation failed."sv);
    return static_cast<uint32_t>(ErrNo::Busy);
  }
  return static_cast<uint32_t>(ErrNo::Success);
}

Expect<uint32_t> GetOutputTensor::body(const Runtime::CallingFrame &Frame,
                                       uint32_t SessionId, uint32_t NamePtr,
                                       uint32_t NameLen, uint32_t TensorIdPtr) {
  // Check memory instance from module.
  MEMINST_CHECK(MemInst, Frame, 0)

  // Get context from ID.
  SESSION_CHECK(Cxt, SessionId, "Invalid session ID."sv, ErrNo::InvalidArgument)

  // Check the input tensor operation name buffer.
  MEM_SV_CHECK(NameSV, MemInst, NamePtr, NameLen,
               "Failed when accessing the output name buffer memory."sv)

  // Check the return value: TensorIdPtr should be valid.
  MEM_PTR_CHECK(TensorId, MemInst, uint32_t, TensorIdPtr,
                "Failed when accessing the return TensorID memory."sv)

  // Find the output tensor.
  bool IsFound = false;
  uint32_t OutCnt = TfLiteInterpreterGetOutputTensorCount(Cxt->Interp);
  for (uint32_t I = 0; I < OutCnt; ++I) {
    const TfLiteTensor *T = TfLiteInterpreterGetOutputTensor(Cxt->Interp, I);
    if (NameSV == std::string(TfLiteTensorName(T))) {
      *TensorId = I;
      IsFound = true;
      break;
    }
  }
  if (unlikely(!IsFound)) {
    spdlog::error("[WasmEdge-Tensorflow-Lite] Output tensor {} not found."sv,
                  NameSV);
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }
  return static_cast<uint32_t>(ErrNo::Success);
}

Expect<uint32_t> GetTensorLen::body(const Runtime::CallingFrame &Frame,
                                    uint32_t SessionId, uint32_t TensorId,
                                    uint32_t LenPtr) {
  // Check memory instance from module.
  MEMINST_CHECK(MemInst, Frame, 0)

  // Get context from ID.
  SESSION_CHECK(Cxt, SessionId, "Invalid session ID."sv, ErrNo::InvalidArgument)

  // Check the return value: LenPtr should be valid.
  MEM_PTR_CHECK(Len, MemInst, uint32_t, LenPtr,
                "Failed when accessing the return Length memory."sv)

  // Get output tensor from ID.
  uint32_t OutCnt = TfLiteInterpreterGetOutputTensorCount(Cxt->Interp);
  if (unlikely(TensorId >= OutCnt)) {
    spdlog::error("[WasmEdge-Tensorflow-Lite] Invalid tensor ID."sv);
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }

  // Return tensor data length.
  const TfLiteTensor *Tensor =
      TfLiteInterpreterGetOutputTensor(Cxt->Interp, TensorId);
  if (likely(Tensor != nullptr)) {
    *Len = TfLiteTensorByteSize(Tensor);
  } else {
    *Len = 0U;
  }
  return static_cast<uint32_t>(ErrNo::Success);
}

Expect<uint32_t> GetTensorData::body(const Runtime::CallingFrame &Frame,
                                     uint32_t SessionId, uint32_t TensorId,
                                     uint32_t BufPtr, uint32_t BufLen,
                                     uint32_t WrittenBytesPtr) {
  // Check memory instance from module.
  MEMINST_CHECK(MemInst, Frame, 0)

  // Get context from ID.
  SESSION_CHECK(Cxt, SessionId, "Invalid session ID."sv, ErrNo::InvalidArgument)

  // Check the output tensor buffer.
  MEM_SPAN_CHECK(
      BufSpan, MemInst, char, BufPtr, BufLen,
      "Failed when accessing the output tensor write buffer memory."sv)

  // Check the return value: WrittenBytesPtr should be valid.
  MEM_PTR_CHECK(WrittenBytes, MemInst, uint32_t, WrittenBytesPtr,
                "Failed when accessing the return WrittenBytes memory."sv)

  // Get output tensor from ID.
  uint32_t OutCnt = TfLiteInterpreterGetOutputTensorCount(Cxt->Interp);
  if (unlikely(TensorId >= OutCnt)) {
    spdlog::error("[WasmEdge-Tensorflow-Lite] Invalid tensor ID."sv);
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }

  // Copy tensor data to buffer.
  const TfLiteTensor *Tensor =
      TfLiteInterpreterGetOutputTensor(Cxt->Interp, TensorId);
  size_t RealSize = TfLiteTensorByteSize(Tensor);
  *WrittenBytes = 0U;
  if (unlikely(RealSize != BufLen)) {
    spdlog::error(
        "[WasmEdge-Tensorflow-Lite] Unexpected buffer length: {}, output tensor size: {}."sv,
        BufLen, RealSize);
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }
  if (likely(Tensor != nullptr)) {
    TfLiteTensorCopyToBuffer(Tensor, BufSpan.data(), RealSize);
  }
  return static_cast<uint32_t>(ErrNo::Success);
}

Expect<uint32_t> AppendInput::body(const Runtime::CallingFrame &Frame,
                                   uint32_t SessionId, uint32_t NamePtr,
                                   uint32_t NameLen, uint32_t TensorBufPtr,
                                   uint32_t TensorBufLen) {
  // Check memory instance from module.
  MEMINST_CHECK(MemInst, Frame, 0)

  // Get context from ID.
  SESSION_CHECK(Cxt, SessionId, "Invalid session ID."sv, ErrNo::InvalidArgument)

  // Check the input tensor buffer.
  MEM_SPAN_CHECK(TensorBufSpan, MemInst, uint8_t, TensorBufPtr, TensorBufLen,
                 "Failed when accessing the input tensor buffer memory."sv)

  // Check the input tensor operation name buffer.
  MEM_SV_CHECK(NameSV, MemInst, NamePtr, NameLen,
               "Failed when accessing the input name buffer memory."sv)

  // Find the input tensor.
  bool IsFound = false;
  uint32_t InCnt = TfLiteInterpreterGetInputTensorCount(Cxt->Interp);
  for (uint32_t I = 0; I < InCnt; ++I) {
    TfLiteTensor *Tensor = TfLiteInterpreterGetInputTensor(Cxt->Interp, I);
    if (NameSV == std::string(TfLiteTensorName(Tensor))) {
      size_t RealSize = TfLiteTensorByteSize(Tensor);
      if (unlikely(RealSize != TensorBufLen)) {
        spdlog::error(
            "[WasmEdge-Tensorflow-Lite] Unexpected buffer length: {}, "
            "input tensor size: {}."sv,
            TensorBufLen, RealSize);
        return static_cast<uint32_t>(ErrNo::InvalidArgument);
      }
      TfLiteStatus Stat = TfLiteTensorCopyFromBuffer(
          Tensor, TensorBufSpan.data(), TensorBufLen);
      if (unlikely(Stat != TfLiteStatus::kTfLiteOk)) {
        spdlog::error(
            "[WasmEdge-Tensorflow-Lite] Copy data from tensor {} failed."sv,
            NameSV);
        return static_cast<uint32_t>(ErrNo::Busy);
      }
      IsFound = true;
      break;
    }
  }
  if (unlikely(!IsFound)) {
    spdlog::error("[WasmEdge-Tensorflow-Lite] Input tensor {} not found."sv,
                  NameSV);
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }

  return static_cast<uint32_t>(ErrNo::Success);
}

} // namespace WasmEdgeTensorflowLite
} // namespace Host
} // namespace WasmEdge
