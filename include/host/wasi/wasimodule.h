// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "host/wasi/environ.h"
#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasiModule : public Runtime::Instance::ModuleInstance {
public:
  WasiModule();

  WASI::Environ &getEnv() noexcept { return Env; }
  const WASI::Environ &getEnv() const noexcept { return Env; }

private:
  WASI::Environ Env;
};

} // namespace Host
} // namespace WasmEdge
