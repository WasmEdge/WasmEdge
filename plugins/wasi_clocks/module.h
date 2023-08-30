// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#pragma once

#include "env.h"

#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeWasiClocksModule : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeWasiClocksModule();

  WasmEdgeWasiClocksEnvironment &getEnv() { return Env; }

private:
  WasmEdgeWasiClocksEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
