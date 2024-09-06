// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "module.h"
#include "func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasiCliEnvironmentModule::WasiCliEnvironmentModule()
    : ComponentInstance("wasi:cli/environment@0.2.0") {}

WasiCliExitModule::WasiCliExitModule()
    : ComponentInstance("wasi:cli/exit@0.2.0") {}

WasiCliStdinModule::WasiCliStdinModule()
    : ComponentInstance("wasi:cli/stdin@0.2.0") {}

WasiCliStdoutModule::WasiCliStdoutModule()
    : ComponentInstance("wasi:cli/stdout@0.2.0") {}

WasiCliStderrModule::WasiCliStderrModule()
    : ComponentInstance("wasi:cli/stderr@0.2.0") {}

} // namespace Host
} // namespace WasmEdge
