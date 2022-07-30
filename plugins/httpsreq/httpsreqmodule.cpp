// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "httpsreqmodule.h"
#include "httpsreqfunc.h"

namespace WasmEdge {
namespace Host {

/// Register your functions in module.
HttpsReqModule::HttpsReqModule() : ModuleInstance("httpsreq") {
  addHostFunc("send_data", std::make_unique<SendData>(Env));
  addHostFunc("get_rcv", std::make_unique<HttpsReqGetRcv>(Env));
  addHostFunc("get_rcv_len", std::make_unique<HttpsReqGetRcvLen>(Env));
}

} // namespace Host
} // namespace WasmEdge