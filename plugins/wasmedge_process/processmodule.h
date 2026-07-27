// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "processenv.h"

#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeProcessModule : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeProcessModule();

  WasmEdgeProcessEnvironment &getEnv() { return Env; }

private:
  WasmEdgeProcessEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
