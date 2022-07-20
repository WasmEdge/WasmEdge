// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "common/defines.h"
#include "httpsreqbase.h"

#include <stdio.h>

namespace WasmEdge {
namespace Host {

class SendData
    : public HttpsReq<SendData> {
public:
  SendData(HttpsReqEnvironment &HostEnv)
      : HttpsReq(HostEnv) {}
  Expect<void> body(Runtime::Instance::MemoryInstance *, uint32_t HostPtr, uint32_t HostLen, uint32_t Port, uint32_t BodyPtr, uint32_t BodyLen);
};

} // namespace Host
} // namespace WasmEdge