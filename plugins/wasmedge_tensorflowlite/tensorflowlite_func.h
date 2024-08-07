// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "tensorflowlite_base.h"

#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeTensorflowLite {

class CreateSession : public Func<CreateSession> {
public:
  CreateSession(TFLiteEnv &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t ModBufPtr,
                        uint32_t ModBufLen, uint32_t SessionIdPtr);
};

class DeleteSession : public Func<DeleteSession> {
public:
  DeleteSession(TFLiteEnv &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SessionId);
};

class RunSession : public Func<RunSession> {
public:
  RunSession(TFLiteEnv &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SessionId);
};

class GetOutputTensor : public Func<GetOutputTensor> {
public:
  GetOutputTensor(TFLiteEnv &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SessionId,
                        uint32_t NamePtr, uint32_t NameLen,
                        uint32_t TensorIdPtr);
};

class GetTensorLen : public Func<GetTensorLen> {
public:
  GetTensorLen(TFLiteEnv &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SessionId,
                        uint32_t TensorId, uint32_t LenPtr);
};

class GetTensorData : public Func<GetTensorData> {
public:
  GetTensorData(TFLiteEnv &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SessionId,
                        uint32_t TensorId, uint32_t BufPtr, uint32_t BufLen,
                        uint32_t WrittenBytesPtr);
};

class AppendInput : public Func<AppendInput> {
public:
  AppendInput(TFLiteEnv &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SessionId,
                        uint32_t NamePtr, uint32_t NameLen,
                        uint32_t TensorBufPtr, uint32_t TensorBufLen);
};

} // namespace WasmEdgeTensorflowLite
} // namespace Host
} // namespace WasmEdge
