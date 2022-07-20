// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "httpsreqmodule.h"
#include "httpsreqfunc.h"

namespace WasmEdge {
namespace Host {

/// Register your functions in module.
HttpsReqModule::HttpsReqModule()
    : ModuleInstance("httpsreq") {
  addHostFunc("send_data",
              std::make_unique<SendData>(Env));
}

} // namespace Host
} // namespace WasmEdge