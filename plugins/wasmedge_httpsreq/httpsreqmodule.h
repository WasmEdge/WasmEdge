// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "httpsreqenv.h"
#include "runtime/instance/module.h"
#include <cstdint>

namespace WasmEdge {
namespace Host {

class WasmEdgeHttpsReqModule : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeHttpsReqModule();

  WasmEdgeHttpsReqEnvironment &getEnv() { return Env; }

private:
  WasmEdgeHttpsReqEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
