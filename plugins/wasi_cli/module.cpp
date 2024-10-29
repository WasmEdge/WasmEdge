// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "module.h"
#include "func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasiCliEnvironmentModule::WasiCliEnvironmentModule()
    : ComponentInstance("wasi:cli/environment@0.2.0") {
  addHostFunc("get-environment", std::make_unique<GetEnvironment>(Env));
  addHostFunc("get-arguments", std::make_unique<GetArguments>(Env));
  addHostFunc("initial-cwd", std::make_unique<InitialCwd>(Env));
}

WasiCliExitModule::WasiCliExitModule()
    : ComponentInstance("wasi:cli/exit@0.2.0") {
  addHostFunc("exit", std::make_unique<Exit>(Env));
  addHostFunc("exit-with-code", std::make_unique<ExitWithCode>(Env));
}

// TODO: complete these module
WasiCliStdinModule::WasiCliStdinModule()
    : ComponentInstance("wasi:cli/stdin@0.2.0") {}

WasiCliStdoutModule::WasiCliStdoutModule()
    : ComponentInstance("wasi:cli/stdout@0.2.0") {}

WasiCliStderrModule::WasiCliStderrModule()
    : ComponentInstance("wasi:cli/stderr@0.2.0") {}

} // namespace Host
} // namespace WasmEdge
