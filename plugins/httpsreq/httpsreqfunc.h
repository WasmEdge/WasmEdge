// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "httpsreqbase.h"

namespace WasmEdge {
namespace Host {

class SendData : public HttpsReq<SendData> {
public:
  SendData(HttpsReqEnvironment &HostEnv) : HttpsReq(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &, uint32_t HostPtr,
                    uint32_t HostLen, uint32_t Port, uint32_t BodyPtr,
                    uint32_t BodyLen);
};

class HttpsReqGetRcv : public HttpsReq<HttpsReqGetRcv> {
public:
  HttpsReqGetRcv(HttpsReqEnvironment &HostEnv) : HttpsReq(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &, uint32_t BufPtr);
};

class HttpsReqGetRcvLen : public HttpsReq<HttpsReqGetRcvLen> {
public:
  HttpsReqGetRcvLen(HttpsReqEnvironment &HostEnv) : HttpsReq(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &);
};

} // namespace Host
} // namespace WasmEdge
