// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "runtime/instance/module.h"
#include "zlibenv.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeZlibModule : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeZlibModule();

  WasmEdgeZlibEnvironment &getEnv() { return Env; }

private:
  WasmEdgeZlibEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
