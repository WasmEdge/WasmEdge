// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "tensorflow_func.h"

#include "common/span.h"
#include "common/spdlog.h"

#include "tensorflow/c/c_api.h"

#include <string>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeTensorflow {

using namespace std::literals::string_view_literals;

namespace {

#define MEMINST_CHECK(Out, CallFrame, Index)                                   \
  auto *Out = CallFrame.getMemoryByIndex(Index);                               \
  if (unlikely(Out == nullptr)) {                                              \
    spdlog::error("[WasmEdge-Tensorflow] Memory instance not found."sv);       \
    return static_cast<uint32_t>(ErrNo::MissingMemory);                        \
  }

#define SESSION_CHECK(Out, SessionID, Message, ErrNo)                          \
  auto *Out = Env.getContext(SessionID);                                       \
  if (unlikely(Out == nullptr)) {                                              \
    spdlog::error("[WasmEdge-Tensorflow] "sv Message);                         \
    return static_cast<uint32_t>(ErrNo);                                       \
  }

#define MEM_SPAN_CHECK(OutSpan, MemInst, Type, BufPtr, BufLen, Message)        \
  auto OutSpan = MemInst->getSpan<Type>(BufPtr, BufLen);                       \
  if (unlikely(OutSpan.size() != BufLen)) {                                    \
    spdlog::error("[WasmEdge-Tensorflow] "sv Message);                         \
    return static_cast<uint32_t>(ErrNo::MissingMemory);                        \
  }

#define MEM_SV_CHECK(OutSV, MemInst, BufPtr, BufLen, Message)                  \
  auto OutSV = MemInst->getStringView(BufPtr, BufLen);                         \
  if (unlikely(OutSV.size() != BufLen)) {                                      \
    spdlog::error("[WasmEdge-Tensorflow] "sv Message);                         \
    return static_cast<uint32_t>(ErrNo::MissingMemory);                        \
  }

#define MEM_PTR_CHECK(OutPtr, MemInst, Type, Offset, Message)                  \
  Type *OutPtr = MemInst->getPointer<Type *>(Offset);                          \
  if (unlikely(OutPtr == nullptr)) {                                           \
    spdlog::error("[WasmEdge-Tensorflow] "sv Message);                         \
    return static_cast<uint32_t>(ErrNo::MissingMemory);                        \
  }

std::pair<std::string, int> parseIndex(std::string_view Name) {
  // Check if there's index in the string key.
  size_t Pos = Name.find(":");
  int Idx = 0;
  std::string NameStr;
  if (Pos != std::string::npos) {
    Idx = std::strtol(Name.data() + Pos + 1, nullptr, 10);
    NameStr = Name.substr(0, Pos);
  } else {
    NameStr = Name;
  }
  return std::make_pair(NameStr, Idx);
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

  Cxt->Graph = TF_NewGraph();
  Cxt->Buffer = TF_NewBufferFromString(ModBufSpan.data(), ModBufLen);
  Cxt->GraphOpts = TF_NewImportGraphDefOptions();
  TF_GraphImportGraphDef(Cxt->Graph, Cxt->Buffer, Cxt->GraphOpts, Cxt->Stat);
  if (unlikely(TF_GetCode(Cxt->Stat) != TF_OK)) {
    spdlog::error("[WasmEdge-Tensorflow] Cannot import graph from buffer: {}"sv,
                  TF_Message(Cxt->Stat));
    Env.deleteContext(NewID);
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }

  // Create session.
  Cxt->SessionOpts = TF_NewSessionOptions();
  Cxt->Session = TF_NewSession(Cxt->Graph, Cxt->SessionOpts, Cxt->Stat);
  if (unlikely(TF_GetCode(Cxt->Stat) != TF_OK)) {
    spdlog::error("[WasmEdge-Tensorflow] Unable to create session: {}"sv,
                  TF_Message(Cxt->Stat));
    Env.deleteContext(NewID);
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }

  *SessionId = NewID;
  return static_cast<uint32_t>(ErrNo::Success);
}

Expect<uint32_t> CreateSessionSavedModel::body(
    const Runtime::CallingFrame &Frame, uint32_t PathPtr, uint32_t PathLen,
    uint32_t TagsBufPtr, uint32_t TagsBufLen, uint32_t SessionIdPtr) {
  // Check memory instance from module.
  MEMINST_CHECK(MemInst, Frame, 0)

  // Check the model path buffer.
  MEM_SV_CHECK(PathSV, MemInst, PathPtr, PathLen,
               "Failed when accessing the model path buffer memory."sv)

  // Check the tags buffer.
  struct MetaGraphDefTag {
    uint32_t Ptr;
    uint32_t Len;
  };
  MEM_SPAN_CHECK(TagSpan, MemInst, MetaGraphDefTag, TagsBufPtr, TagsBufLen,
                 "Failed when accessing the tags memory."sv)

  // Check the elements of tags.
  std::vector<std::string> Tags;
  std::vector<const char *> TagsArgv;
  Tags.reserve(TagsBufLen);
  TagsArgv.reserve(TagsBufLen);
  for (size_t I = 0; I < TagSpan.size(); ++I) {
    // Should use std::string to copy the tag name here to prevent from no
    // null-termination of the tag strings here.
    const auto &Tag = TagSpan[I];
    MEM_SV_CHECK(TagNameSV, MemInst, Tag.Ptr, Tag.Len,
                 "Failed when accessing the tag name memory."sv)
    Tags.emplace_back(TagNameSV);
    TagsArgv.emplace_back(Tags.back().c_str());
  }

  // Check the return value: SessionIdPtr should be valid.
  MEM_PTR_CHECK(SessionId, MemInst, uint32_t, SessionIdPtr,
                "Failed when accessing the return SessionID memory."sv)

  // Create context and import graph.
  uint32_t NewID = Env.newContext();
  SESSION_CHECK(Cxt, NewID, "Failed when allocating resources."sv,
                ErrNo::MissingMemory)

  // Create session.
  Cxt->Graph = TF_NewGraph();
  Cxt->GraphOpts = TF_NewImportGraphDefOptions();
  Cxt->SessionOpts = TF_NewSessionOptions();
  Cxt->Session = TF_LoadSessionFromSavedModel(
      Cxt->SessionOpts, nullptr, std::string(PathSV).c_str(), TagsArgv.data(),
      TagsArgv.size(), Cxt->Graph, nullptr, Cxt->Stat);
  if (unlikely(TF_GetCode(Cxt->Stat) != TF_OK)) {
    spdlog::error("[WasmEdge-Tensorflow] Unable to create session: {}"sv,
                  TF_Message(Cxt->Stat));
    Env.deleteContext(NewID);
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
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

  // Delete old output tensors
  for (auto T : Cxt->Outputs.DataList) {
    if (T) {
      TF_DeleteTensor(T);
    }
  }

  // Run session
  TF_SessionRun(Cxt->Session,
                // RunOptions
                nullptr,
                // Input tensors
                Cxt->Inputs.OperList.data(), Cxt->Inputs.DataList.data(),
                Cxt->Inputs.DataList.size(),
                // Output tensors
                Cxt->Outputs.OperList.data(), Cxt->Outputs.DataList.data(),
                Cxt->Outputs.DataList.size(),
                // Target operations
                nullptr, 0,
                // RunMetadata
                nullptr,
                // Output status
                Cxt->Stat);

  if (unlikely(TF_GetCode(Cxt->Stat) != TF_OK)) {
    spdlog::error("[WasmEdge-Tensorflow] Run session failed: {}"sv,
                  TF_Message(Cxt->Stat));
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

  // Find the output tensor ID.
  auto It = Cxt->Outputs.NameMap.find(std::string(NameSV));
  if (unlikely(It == Cxt->Outputs.NameMap.end())) {
    spdlog::error("[WasmEdge-Tensorflow] Output tensor {} not found."sv,
                  NameSV);
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }
  *TensorId = It->second;
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
  if (unlikely(TensorId >= Cxt->Outputs.DataList.size())) {
    spdlog::error("[WasmEdge-Tensorflow] Invalid tensor ID."sv);
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }

  // Return tensor data length.
  auto *Tensor = Cxt->Outputs.DataList[TensorId];
  if (likely(Tensor != nullptr)) {
    *Len = TF_TensorByteSize(Tensor);
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
  if (unlikely(TensorId >= Cxt->Outputs.DataList.size())) {
    spdlog::error("[WasmEdge-Tensorflow] Invalid tensor ID."sv);
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }

  // Copy tensor data to buffer.
  auto *Tensor = Cxt->Outputs.DataList[TensorId];
  size_t RealSize = TF_TensorByteSize(Tensor);
  *WrittenBytes = 0U;
  if (Tensor != nullptr && RealSize > 0 && BufLen > 0) {
    *WrittenBytes = std::min(static_cast<uint32_t>(RealSize), BufLen);
    char *Data = static_cast<char *>(TF_TensorData(Tensor));
    std::copy_n(Data, *WrittenBytes, BufSpan.data());
  }
  return static_cast<uint32_t>(ErrNo::Success);
}

Expect<uint32_t> AppendInput::body(const Runtime::CallingFrame &Frame,
                                   uint32_t SessionId, uint32_t NamePtr,
                                   uint32_t NameLen, uint32_t DimPtr,
                                   uint32_t DimCnt, uint32_t DataType,
                                   uint32_t TensorBufPtr,
                                   uint32_t TensorBufLen) {
  // Check memory instance from module.
  MEMINST_CHECK(MemInst, Frame, 0)

  // Get context from ID.
  SESSION_CHECK(Cxt, SessionId, "Invalid session ID."sv, ErrNo::InvalidArgument)

  // Check the input tensor buffer.
  MEM_SPAN_CHECK(TensorBufSpan, MemInst, uint8_t, TensorBufPtr, TensorBufLen,
                 "Failed when accessing the input tensor buffer memory."sv)

  // Check the input tensor dimension buffer.
  MEM_SPAN_CHECK(DimBufSpan, MemInst, int64_t, DimPtr, DimCnt,
                 "Failed when accessing the input dimension buffer memory."sv)

  // Check the input tensor operation name buffer.
  MEM_SV_CHECK(NameSV, MemInst, NamePtr, NameLen,
               "Failed when accessing the input name buffer memory."sv)

  // Check the input operation.
  auto OperKeyPair = parseIndex(NameSV);
  TF_Operation *Operation =
      TF_GraphOperationByName(Cxt->Graph, OperKeyPair.first.c_str());
  if (unlikely(Operation == nullptr)) {
    spdlog::error("[WasmEdge-Tensorflow] Input operation {} not found."sv,
                  NameSV);
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }

  // Check if the input tensor by name exists.
  uint32_t TensorId = Cxt->Inputs.DataList.size();
  auto It = Cxt->Inputs.NameMap.find(std::string(NameSV));
  if (It != Cxt->Inputs.NameMap.end()) {
    TensorId = It->second;
  }

  // Create the tensor and copy data from buffer.
  TF_Tensor *Tensor = nullptr;
  if (DimCnt > 0) {
    Tensor = TF_AllocateTensor(static_cast<TF_DataType>(DataType),
                               DimBufSpan.data(), DimCnt, TensorBufLen);
  } else {
    Tensor = TF_AllocateTensor(static_cast<TF_DataType>(DataType), nullptr, 0,
                               TensorBufLen);
  }
  if (unlikely(Tensor == nullptr)) {
    spdlog::error("[WasmEdge-Tensorflow] Allocate input tensor failed."sv);
    return static_cast<uint32_t>(ErrNo::Busy);
  }
  std::copy_n(TensorBufSpan.begin(), TensorBufLen,
              static_cast<uint8_t *>(TF_TensorData(Tensor)));

  // If the old input tensor exists, delete the old one.
  if (It != Cxt->Inputs.NameMap.end()) {
    TF_DeleteTensor(Cxt->Inputs.DataList[TensorId]);
    Cxt->Inputs.DataList[TensorId] = Tensor;
  } else {
    Cxt->Inputs.OperList.emplace_back(TF_Output{Operation, OperKeyPair.second});
    Cxt->Inputs.DataList.push_back(Tensor);
    Cxt->Inputs.NameMap.insert({std::string(NameSV), TensorId});
  }
  return static_cast<uint32_t>(ErrNo::Success);
}

Expect<uint32_t> AppendOutput::body(const Runtime::CallingFrame &Frame,
                                    uint32_t SessionId, uint32_t NamePtr,
                                    uint32_t NameLen) {
  // Check memory instance from module.
  MEMINST_CHECK(MemInst, Frame, 0)

  // Get context from ID.
  SESSION_CHECK(Cxt, SessionId, "Invalid session ID."sv, ErrNo::InvalidArgument)

  // Check the output tensor operation name buffer.
  MEM_SV_CHECK(NameSV, MemInst, NamePtr, NameLen,
               "Failed when accessing the output name buffer memory."sv)

  // Check the output operation.
  auto OperKeyPair = parseIndex(NameSV);
  TF_Operation *Operation =
      TF_GraphOperationByName(Cxt->Graph, OperKeyPair.first.c_str());
  if (unlikely(Operation == nullptr)) {
    spdlog::error("[WasmEdge-Tensorflow] Output operation {} not found."sv,
                  NameSV);
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }

  // Store names and operations if the output tensor key not exists.
  auto It = Cxt->Outputs.NameMap.find(std::string(NameSV));
  if (It == Cxt->Outputs.NameMap.end()) {
    uint32_t TensorId = Cxt->Outputs.DataList.size();
    Cxt->Outputs.OperList.emplace_back(
        TF_Output{Operation, OperKeyPair.second});
    Cxt->Outputs.DataList.push_back(nullptr);
    Cxt->Outputs.NameMap.insert({std::string(NameSV), TensorId});
  }
  return static_cast<uint32_t>(ErrNo::Success);
}

Expect<uint32_t> ClearInput::body(const Runtime::CallingFrame &,
                                  uint32_t SessionId) {
  // Get context from ID.
  SESSION_CHECK(Cxt, SessionId, "Invalid session ID."sv, ErrNo::InvalidArgument)

  // Clear the inputs.
  Cxt->clearInputs();
  return static_cast<uint32_t>(ErrNo::Success);
}

Expect<uint32_t> ClearOutput::body(const Runtime::CallingFrame &,
                                   uint32_t SessionId) {
  // Get context from ID.
  SESSION_CHECK(Cxt, SessionId, "Invalid session ID."sv, ErrNo::InvalidArgument)

  // Clear the outputs.
  Cxt->clearOutputs();
  return static_cast<uint32_t>(ErrNo::Success);
}

} // namespace WasmEdgeTensorflow
} // namespace Host
} // namespace WasmEdge
