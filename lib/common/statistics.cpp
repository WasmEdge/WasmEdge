// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/statistics.h"
#include "common/configure.h"
#include <chrono>
#include <cmath>
#include <limits>
#include <string>
#include <string_view>
#include <fmt/core.h>
#include <spdlog/spdlog.h>

namespace WasmEdge {
namespace Statistics {

void Statistics::dumpToLog(const Configure &Conf) const noexcept {
  using namespace std::literals;
  auto Nano = [](auto &&Duration) {
    return std::chrono::nanoseconds(Duration).count();
  };
  const auto &StatConf = Conf.getStatisticsConfigure();
  const bool HasTime = StatConf.isTimeMeasuring();
  const bool HasCount = StatConf.isInstructionCounting();
  const bool HasCost = StatConf.isCostMeasuring();
  const bool AnyStatEnabled = HasTime || HasCount || HasCost;

  if (StatConf.getStatsOutputFormat() ==
      Configure::StatisticsConfigure::StatsOutputFormat::JSON) {
    if (!AnyStatEnabled) {
      fmt::print(stderr,
                 "--stats-format=json has no effect without --enable-time-measuring, --enable-instruction-count, or --enable-gas-measuring.\n"sv);
      return;
    }
    std::string Json = "{\n";
    bool First = true;
    auto AddField = [&](std::string_view Key, uint64_t Val) {
      if (!First) {
        Json += ",\n";
      }
      First = false;
      Json += fmt::format("  \"{}\": {}", Key, Val);
    };
    if (HasTime) {
      AddField("total_execution_time_ns",
               static_cast<uint64_t>(Nano(getTotalExecTime())));
      AddField("wasm_instruction_time_ns",
               static_cast<uint64_t>(Nano(getWasmExecTime())));
      AddField("host_function_time_ns",
               static_cast<uint64_t>(Nano(getHostFuncExecTime())));
    }
    if (HasCount) {
      AddField("instruction_count", getInstrCount());
    }
    if (HasCost) {
      AddField("gas_used", getTotalCost());
    }
    if (HasCount && HasTime) {
      const double IPS = getInstrPerSecond();
      const uint64_t IPSVal =
          likely(std::isfinite(IPS)) ? static_cast<uint64_t>(IPS)
                                   : std::numeric_limits<uint64_t>::max();
      AddField("instructions_per_second", IPSVal);
    }
    Json += "\n}\n";
    fmt::print("{}", Json);
    return;
  }

  if (AnyStatEnabled) {
    spdlog::info("====================  Statistics  ===================="sv);
  }
  if (HasTime) {
    spdlog::info(" Total execution time: {} ns"sv, Nano(getTotalExecTime()));
    spdlog::info(" Wasm instructions execution time: {} ns"sv,
                 Nano(getWasmExecTime()));
    spdlog::info(" Host functions execution time: {} ns"sv,
                 Nano(getHostFuncExecTime()));
  }
  if (HasCount) {
    spdlog::info(" Executed wasm instructions count: {}"sv, getInstrCount());
  }
  if (HasCost) {
    spdlog::info(" Gas costs: {}"sv, getTotalCost());
  }
  if (HasCount && HasTime) {
    const double IPS = getInstrPerSecond();
    spdlog::info(" Instructions per second: {}"sv,
                 likely(std::isfinite(IPS))
                     ? static_cast<uint64_t>(IPS)
                     : std::numeric_limits<uint64_t>::max());
  }
  if (AnyStatEnabled) {
    spdlog::info("=======================   End   ======================"sv);
  }
}

} // namespace Statistics
} // namespace WasmEdge
