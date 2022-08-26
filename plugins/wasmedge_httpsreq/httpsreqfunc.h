// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "common/defines.h"
#include "httpsreqbase.h"

#include <stdio.h>

namespace WasmEdge {
namespace Host {

class WasmEdgeHttpsReqSendData
    : public WasmEdgeHttpsReq<WasmEdgeHttpsReqSendData> {
public:
  WasmEdgeHttpsReqSendData(WasmEdgeHttpsReqEnvironment &HostEnv)
      : WasmEdgeHttpsReq(HostEnv) {}
  Expect<void> body(Runtime::Instance::MemoryInstance *, uint32_t HostPtr,
                    uint32_t HostLen, uint32_t Port, uint32_t BodyPtr,
                    uint32_t BodyLen);
};

class WasmEdgeHttpsReqGetRcv : public WasmEdgeHttpsReq<WasmEdgeHttpsReqGetRcv> {
public:
  WasmEdgeHttpsReqGetRcv(WasmEdgeHttpsReqEnvironment &HostEnv)
      : WasmEdgeHttpsReq(HostEnv) {}
  Expect<void> body(Runtime::Instance::MemoryInstance *, uint32_t BufPtr);
};

class WasmEdgeHttpsReqGetRcvLen
    : public WasmEdgeHttpsReq<WasmEdgeHttpsReqGetRcvLen> {
public:
  WasmEdgeHttpsReqGetRcvLen(WasmEdgeHttpsReqEnvironment &HostEnv)
      : WasmEdgeHttpsReq(HostEnv) {}
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *);
};

} // namespace Host
} // namespace WasmEdge