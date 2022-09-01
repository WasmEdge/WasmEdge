// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "httpsreqmodule.h"
#include "httpsreqfunc.h"

namespace WasmEdge {
namespace Host {

/// Register your functions in module.
WasmEdgeHttpsReqModule::WasmEdgeHttpsReqModule()
    : ModuleInstance("wasmedge_httpsreq") {
  addHostFunc("wasmedge_httpsreq_send_data",
              std::make_unique<WasmEdgeHttpsReqSendData>(Env));
  addHostFunc("wasmedge_httpsreq_get_rcv",
              std::make_unique<WasmEdgeHttpsReqGetRcv>(Env));
  addHostFunc("wasmedge_httpsreq_get_rcv_len",
              std::make_unique<WasmEdgeHttpsReqGetRcvLen>(Env));
}

} // namespace Host
} // namespace WasmEdge
