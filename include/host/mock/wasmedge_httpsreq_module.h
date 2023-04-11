// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "host/mock/wasmedge_httpsreq_func.h"
#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeHttpsReqModuleMock : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeHttpsReqModuleMock()
      : Runtime::Instance::ModuleInstance("wasmedge_httpsreq") {
    addHostFunc("wasmedge_httpsreq_send_data",
                std::make_unique<WasmEdgeHttpsReqMock::SendData>());
    addHostFunc("wasmedge_httpsreq_get_rcv",
                std::make_unique<WasmEdgeHttpsReqMock::GetRcv>());
    addHostFunc("wasmedge_httpsreq_get_rcv_len",
                std::make_unique<WasmEdgeHttpsReqMock::GetRcvLen>());
  }
};

} // namespace Host
} // namespace WasmEdge
