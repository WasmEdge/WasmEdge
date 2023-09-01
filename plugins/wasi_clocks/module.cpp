// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#include "module.h"
#include "func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasmEdgeWasiClocksModule::WasmEdgeWasiClocksModule()
    : ModuleInstance("wasmedge_wasi_clocks") {

  addHostFunc("wasi_clocks_monotonic_clock_now",
              std::make_unique<MonotonicClock::Now>(Env));
  addHostFunc("wasi_clocks_monotonic_clock_resolution",
              std::make_unique<MonotonicClock::Resolution>(Env));

  addHostFunc("wasi_clocks_timezone_utc_offset",
              std::make_unique<Timezone::UtcOffset>(Env));

  addHostFunc("wasi_clocks_wall_clock_now",
              std::make_unique<WallClock::Now>(Env));
  addHostFunc("wasi_clocks_wall_clock_resolution",
              std::make_unique<WallClock::Resolution>(Env));
}

} // namespace Host
} // namespace WasmEdge
