// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#include "module.h"
#include "func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasmEdgeWasiClocksModule::WasmEdgeWasiClocksModule()
    : ModuleInstance("wasmedge_wasi_clocks") {}

} // namespace Host
} // namespace WasmEdge
