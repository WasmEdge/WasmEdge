// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "runtime/instance/module.h"
#include "httpsreqenv.h"
#include <cstdint>

namespace WasmEdge {
namespace Host {

class HttpsReqModule : public Runtime::Instance::ModuleInstance {
public:
  HttpsReqModule();

  HttpsReqEnvironment &getEnv() { return Env; }

private:
  HttpsReqEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge