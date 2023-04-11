// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "common/errcode.h"
#include "host/mock/log.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeHttpsReqMock {

using namespace std::literals;

class SendData : public Runtime::HostFunction<SendData> {
public:
  Expect<void> body(const Runtime::CallingFrame &, uint32_t, uint32_t, uint32_t,
                    uint32_t, uint32_t) {
    printPluginMock("WasmEdge-HttpsReq"sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
};

class GetRcv : public Runtime::HostFunction<GetRcv> {
public:
  Expect<void> body(const Runtime::CallingFrame &, uint32_t) {
    printPluginMock("WasmEdge-HttpsReq"sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
};

class GetRcvLen : public Runtime::HostFunction<GetRcvLen> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &) {
    printPluginMock("WasmEdge-HttpsReq"sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
};

} // namespace WasmEdgeHttpsReqMock
} // namespace Host
} // namespace WasmEdge
