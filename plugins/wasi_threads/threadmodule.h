// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "base.h"
#include "runtime/instance/module.h"
#include "threadenv.h"

namespace WasmEdge {
namespace Host {

class WasiThreadsModule : public Runtime::Instance::ModuleInstance {
public:
  WasiThreadsModule();

  WasiThreadsEnvironment &getEnv() noexcept { return Env; }
  const WasiThreadsEnvironment &getEnv() const noexcept { return Env; }

private:
  WasiThreadsEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
