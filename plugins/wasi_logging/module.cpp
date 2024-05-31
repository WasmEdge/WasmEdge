// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "module.h"
#include "func.h"

#include <string_view>

namespace WasmEdge {
namespace Host {

using namespace std::literals;

WasiLoggingModule::WasiLoggingModule()
    : ModuleInstance("wasi:logging/logging"sv) {
  addHostFunc("log"sv, std::make_unique<WASILogging::Log>(Env));
}

} // namespace Host
} // namespace WasmEdge
