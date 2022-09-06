// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "runtime/instance/module.h"
#include "threadenv.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeThreadModule : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeThreadModule();

  WasmEdgeThreadEnvironment &getEnv() noexcept { return Env; }
  const WasmEdgeThreadEnvironment &getEnv() const noexcept { return Env; }

private:
  WasmEdgeThreadEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
