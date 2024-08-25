// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "tensorflow_base.h"

#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeTensorflow {

class CreateSession : public Func<CreateSession> {
public:
  CreateSession(TFEnv &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t ModBufPtr,
                        uint32_t ModBufLen, uint32_t SessionIdPtr);
};

class CreateSessionSavedModel : public Func<CreateSessionSavedModel> {
public:
  CreateSessionSavedModel(TFEnv &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t PathPtr,
                        uint32_t PathLen, uint32_t TagsBufPtr,
                        uint32_t TagsBufLen, uint32_t SessionIdPtr);
};

class DeleteSession : public Func<DeleteSession> {
public:
  DeleteSession(TFEnv &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SessionId);
};

class RunSession : public Func<RunSession> {
public:
  RunSession(TFEnv &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SessionId);
};

class GetOutputTensor : public Func<GetOutputTensor> {
public:
  GetOutputTensor(TFEnv &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SessionId,
                        uint32_t NamePtr, uint32_t NameLen,
                        uint32_t TensorIdPtr);
};

class GetTensorLen : public Func<GetTensorLen> {
public:
  GetTensorLen(TFEnv &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SessionId,
                        uint32_t TensorId, uint32_t LenPtr);
};

class GetTensorData : public Func<GetTensorData> {
public:
  GetTensorData(TFEnv &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SessionId,
                        uint32_t TensorId, uint32_t BufPtr, uint32_t BufLen,
                        uint32_t WrittenBytesPtr);
};

class AppendInput : public Func<AppendInput> {
public:
  AppendInput(TFEnv &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SessionId,
                        uint32_t NamePtr, uint32_t NameLen, uint32_t DimPtr,
                        uint32_t DimCnt, uint32_t DataType,
                        uint32_t TensorBufPtr, uint32_t TensorBufLen);
};

class AppendOutput : public Func<AppendOutput> {
public:
  AppendOutput(TFEnv &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SessionId,
                        uint32_t NamePtr, uint32_t NameLen);
};

class ClearInput : public Func<ClearInput> {
public:
  ClearInput(TFEnv &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SessionId);
};

class ClearOutput : public Func<ClearOutput> {
public:
  ClearOutput(TFEnv &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SessionId);
};

} // namespace WasmEdgeTensorflow
} // namespace Host
} // namespace WasmEdge
