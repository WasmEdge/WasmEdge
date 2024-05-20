// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "env.h"

#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasiLoggingModule : public Runtime::Instance::ModuleInstance {
public:
  WasiLoggingModule();

  WASILogging::LogEnv &getEnv() { return Env; }

private:
  WASILogging::LogEnv Env;
};

} // namespace Host
} // namespace WasmEdge
