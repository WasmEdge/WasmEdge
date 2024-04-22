// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "env.h"

#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasiPollModule : public Runtime::Instance::ComponentInstance {
public:
  WasiPollModule();

  WasiPollEnvironment &getEnv() { return Env; }

private:
  WasiPollEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
