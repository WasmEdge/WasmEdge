// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "common/errcode.h"
#include "host/mock/log.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeTensorflowMock {

using namespace std::literals;
static inline constexpr const uint32_t kWasmEdgeTensorflowError = 1U;

class CreateSession : public Runtime::HostFunction<CreateSession> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t) {
    printPluginMock("WasmEdge-Tensorflow"sv);
    return kWasmEdgeTensorflowError;
  }
};

class DeleteSession : public Runtime::HostFunction<DeleteSession> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t) {
    printPluginMock("WasmEdge-Tensorflow"sv);
    return kWasmEdgeTensorflowError;
  }
};

class RunSession : public Runtime::HostFunction<RunSession> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t) {
    printPluginMock("WasmEdge-Tensorflow"sv);
    return kWasmEdgeTensorflowError;
  }
};

class GetOutputTensor : public Runtime::HostFunction<GetOutputTensor> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, uint32_t) {
    printPluginMock("WasmEdge-Tensorflow"sv);
    return kWasmEdgeTensorflowError;
  }
};

class GetTensorLen : public Runtime::HostFunction<GetTensorLen> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t) {
    printPluginMock("WasmEdge-Tensorflow"sv);
    return kWasmEdgeTensorflowError;
  }
};

class GetTensorData : public Runtime::HostFunction<GetTensorData> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t) {
    printPluginMock("WasmEdge-Tensorflow"sv);
    return kWasmEdgeTensorflowError;
  }
};

class AppendInput : public Runtime::HostFunction<AppendInput> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                        uint32_t) {
    printPluginMock("WasmEdge-Tensorflow"sv);
    return kWasmEdgeTensorflowError;
  }
};

class AppendOutput : public Runtime::HostFunction<AppendOutput> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t) {
    printPluginMock("WasmEdge-Tensorflow"sv);
    return kWasmEdgeTensorflowError;
  }
};

class ClearInput : public Runtime::HostFunction<ClearInput> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t) {
    printPluginMock("WasmEdge-Tensorflow"sv);
    return kWasmEdgeTensorflowError;
  }
};

class ClearOutput : public Runtime::HostFunction<ClearOutput> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t) {
    printPluginMock("WasmEdge-Tensorflow"sv);
    return kWasmEdgeTensorflowError;
  }
};

} // namespace WasmEdgeTensorflowMock
} // namespace Host
} // namespace WasmEdge
