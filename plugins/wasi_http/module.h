// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "env.h"

#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasiHttpModule : public Runtime::Instance::ComponentInstance {
public:
  WasiHttpModule();

  WasiHttpEnvironment &getEnv() { return Env; }

private:
  WasiHttpEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
