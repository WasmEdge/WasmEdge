// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "common/errcode.h"
#include "host/mock/log.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeProcessMock {

using namespace std::literals;

class SetProgName : public Runtime::HostFunction<SetProgName> {
public:
  Expect<void> body(const Runtime::CallingFrame &, uint32_t, uint32_t) {
    printPluginMock("WasmEdge-Process"sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
};

class AddArg : public Runtime::HostFunction<AddArg> {
public:
  Expect<void> body(const Runtime::CallingFrame &, uint32_t, uint32_t) {
    printPluginMock("WasmEdge-Process"sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
};

class AddEnv : public Runtime::HostFunction<AddEnv> {
public:
  Expect<void> body(const Runtime::CallingFrame &, uint32_t, uint32_t, uint32_t,
                    uint32_t) {
    printPluginMock("WasmEdge-Process"sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
};

class AddStdIn : public Runtime::HostFunction<AddStdIn> {
public:
  Expect<void> body(const Runtime::CallingFrame &, uint32_t, uint32_t) {
    printPluginMock("WasmEdge-Process"sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
};

class SetTimeOut : public Runtime::HostFunction<SetTimeOut> {
public:
  Expect<void> body(const Runtime::CallingFrame &, uint32_t) {
    printPluginMock("WasmEdge-Process"sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
};

class Run : public Runtime::HostFunction<Run> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &) {
    printPluginMock("WasmEdge-Process"sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
};

class GetExitCode : public Runtime::HostFunction<GetExitCode> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &) {
    printPluginMock("WasmEdge-Process"sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
};

class GetStdOutLen : public Runtime::HostFunction<GetStdOutLen> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &) {
    printPluginMock("WasmEdge-Process"sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
};

class GetStdOut : public Runtime::HostFunction<GetStdOut> {
public:
  Expect<void> body(const Runtime::CallingFrame &, uint32_t) {
    printPluginMock("WasmEdge-Process"sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
};

class GetStdErrLen : public Runtime::HostFunction<GetStdErrLen> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &) {
    printPluginMock("WasmEdge-Process"sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
};

class GetStdErr : public Runtime::HostFunction<GetStdErr> {
public:
  Expect<void> body(const Runtime::CallingFrame &, uint32_t) {
    printPluginMock("WasmEdge-Process"sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
};

} // namespace WasmEdgeProcessMock
} // namespace Host
} // namespace WasmEdge
