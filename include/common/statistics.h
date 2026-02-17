// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/statistics.h - Executor statistics definition -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the statistics class of runtime.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/configure.h"
#include "common/cputime.h"
#include "common/enum_ast.hpp"
#include "common/errcode.h"
#include "common/span.h"
#include "common/spdlog.h"
#include "common/timer.h"

#include <atomic>
#include <vector>

namespace WasmEdge {
namespace Statistics {

class Statistics {
public:
  Statistics(const uint64_t Lim = UINT64_MAX)
      : CostTab(UINT16_MAX + 1, 1ULL), InstrCnt(0), CostLimit(Lim), CostSum(0) {
  }
  Statistics(Span<const uint64_t> Tab, const uint64_t Lim = UINT64_MAX)
      : CostTab(Tab.begin(), Tab.end()), InstrCnt(0), CostLimit(Lim),
        CostSum(0) {
    if (CostTab.size() < UINT16_MAX + 1) {
      CostTab.resize(UINT16_MAX + 1, 0ULL);
    }
  }
  ~Statistics() = default;

  /// Increment of instruction counter.
  void incInstrCount() { InstrCnt.fetch_add(1, std::memory_order_relaxed); }

  /// Getter of instruction counter.
  uint64_t getInstrCount() const {
    return InstrCnt.load(std::memory_order_relaxed);
  }
  std::atomic_uint64_t &getInstrCountRef() { return InstrCnt; }

  /// Getter of instruction per second.
  double getInstrPerSecond() const {
    return static_cast<double>(InstrCnt) /
           std::chrono::duration<double>(getWasmExecTime()).count();
  }

  /// Setter and setter of cost table.
  void setCostTable(Span<const uint64_t> NewTable) {
    CostTab.assign(NewTable.begin(), NewTable.end());
    if (unlikely(CostTab.size() < UINT16_MAX + 1)) {
      CostTab.resize(UINT16_MAX + 1, 0ULL);
    }
  }
  Span<const uint64_t> getCostTable() const noexcept { return CostTab; }
  Span<uint64_t> getCostTable() noexcept { return CostTab; }

  /// Adder of instruction costs.
  bool addInstrCost(OpCode Code) { return addCost(CostTab[uint16_t(Code)]); }

  /// Subber of instruction costs.
  bool subInstrCost(OpCode Code) { return subCost(CostTab[uint16_t(Code)]); }

  /// Getter of total gas cost.
  uint64_t getTotalCost() const {
    return CostSum.load(std::memory_order_relaxed);
  }
  std::atomic_uint64_t &getTotalCostRef() { return CostSum; }

  /// Getter and setter of cost limit.
  void setCostLimit(uint64_t Lim) { CostLimit = Lim; }
  uint64_t getCostLimit() const { return CostLimit; }

  /// Add cost and return false if exceeded limit.
  bool addCost(uint64_t Cost) {
    using namespace std::literals;
    const auto Limit = CostLimit;
    uint64_t OldCostSum = CostSum.load(std::memory_order_relaxed);
    uint64_t NewCostSum;
    do {
      NewCostSum = OldCostSum + Cost;
      if (unlikely(NewCostSum > Limit)) {
        spdlog::error("Cost exceeded limit. Force terminate the execution."sv);
        return false;
      }
    } while (!CostSum.compare_exchange_weak(OldCostSum, NewCostSum,
                                            std::memory_order_relaxed));
    return true;
  }

  /// Return cost back.
  bool subCost(uint64_t Cost) {
    uint64_t OldCostSum = CostSum.load(std::memory_order_relaxed);
    uint64_t NewCostSum;
    do {
      if (unlikely(OldCostSum <= Cost)) {
        return false;
      }
      NewCostSum = OldCostSum - Cost;
    } while (!CostSum.compare_exchange_weak(OldCostSum, NewCostSum,
                                            std::memory_order_relaxed));
    return true;
  }

  /// Clear measurement data for instructions.
  void clear() noexcept {
    TimeRecorder.reset();
    InstrCnt.store(0, std::memory_order_relaxed);
    CostSum.store(0, std::memory_order_relaxed);
    CpuTimeStartNs.store(0, std::memory_order_relaxed);
    CpuTimeTotalNs.store(0, std::memory_order_relaxed);
    WasmMemoryPages.store(0, std::memory_order_relaxed);
    WasmMemoryPeakPages.store(0, std::memory_order_relaxed);
  }

  /// Start recording wasm time.
  void startRecordWasm() noexcept {
    TimeRecorder.startRecord(Timer::TimerTag::Wasm);
  }

  /// Stop recording wasm time.
  void stopRecordWasm() noexcept {
    TimeRecorder.stopRecord(Timer::TimerTag::Wasm);
  }

  /// Start recording host function time.
  void startRecordHost() noexcept {
    TimeRecorder.startRecord(Timer::TimerTag::HostFunc);
  }

  /// Stop recording host function time.
  void stopRecordHost() noexcept {
    TimeRecorder.stopRecord(Timer::TimerTag::HostFunc);
  }

  /// Getter of execution time.
  Timer::Timer::Clock::duration getWasmExecTime() const noexcept {
    return TimeRecorder.getRecord(Timer::TimerTag::Wasm);
  }
  Timer::Timer::Clock::duration getHostFuncExecTime() const noexcept {
    return TimeRecorder.getRecord(Timer::TimerTag::HostFunc);
  }
  Timer::Timer::Clock::duration getTotalExecTime() const noexcept {
    return TimeRecorder.getRecord(Timer::TimerTag::Wasm) +
           TimeRecorder.getRecord(Timer::TimerTag::HostFunc);
  }

  /// Start/stop recording cold start load time.
  void startRecordColdStartLoad() noexcept {
    TimeRecorder.startRecord(Timer::TimerTag::ColdStartLoad);
  }
  void stopRecordColdStartLoad() noexcept {
    TimeRecorder.stopRecord(Timer::TimerTag::ColdStartLoad);
  }

  /// Start/stop recording cold start validate time.
  void startRecordColdStartValidate() noexcept {
    TimeRecorder.startRecord(Timer::TimerTag::ColdStartValidate);
  }
  void stopRecordColdStartValidate() noexcept {
    TimeRecorder.stopRecord(Timer::TimerTag::ColdStartValidate);
  }

  /// Start/stop recording cold start instantiate time.
  void startRecordColdStartInstantiate() noexcept {
    TimeRecorder.startRecord(Timer::TimerTag::ColdStartInstantiate);
  }
  void stopRecordColdStartInstantiate() noexcept {
    TimeRecorder.stopRecord(Timer::TimerTag::ColdStartInstantiate);
  }

  /// Getter of cold start phase times.
  Timer::Timer::Clock::duration getColdStartLoadTime() const noexcept {
    return TimeRecorder.getRecord(Timer::TimerTag::ColdStartLoad);
  }
  Timer::Timer::Clock::duration getColdStartValidateTime() const noexcept {
    return TimeRecorder.getRecord(Timer::TimerTag::ColdStartValidate);
  }
  Timer::Timer::Clock::duration getColdStartInstantiateTime() const noexcept {
    return TimeRecorder.getRecord(Timer::TimerTag::ColdStartInstantiate);
  }
  Timer::Timer::Clock::duration getColdStartTotalTime() const noexcept {
    return getColdStartLoadTime() + getColdStartValidateTime() +
           getColdStartInstantiateTime();
  }

  /// Start recording process CPU time.
  void startRecordCpuTime() noexcept {
    CpuTimeStartNs.store(CpuTime::getProcessCpuTimeNs(),
                         std::memory_order_relaxed);
  }

  /// Stop recording process CPU time and accumulate.
  void stopRecordCpuTime() noexcept {
    uint64_t Start = CpuTimeStartNs.load(std::memory_order_relaxed);
    uint64_t Now = CpuTime::getProcessCpuTimeNs();
    if (Now > Start) {
      CpuTimeTotalNs.fetch_add(Now - Start, std::memory_order_relaxed);
    }
  }

  /// Getter of accumulated CPU time in nanoseconds.
  uint64_t getCpuTimeNs() const noexcept {
    return CpuTimeTotalNs.load(std::memory_order_relaxed);
  }

  /// Record current Wasm memory pages and update peak.
  void recordMemoryPages(uint64_t Pages) noexcept {
    WasmMemoryPages.store(Pages, std::memory_order_relaxed);
    uint64_t OldPeak = WasmMemoryPeakPages.load(std::memory_order_relaxed);
    while (Pages > OldPeak) {
      if (WasmMemoryPeakPages.compare_exchange_weak(OldPeak, Pages,
                                                    std::memory_order_relaxed))
        break;
    }
  }

  /// Getter of current Wasm memory pages.
  uint64_t getMemoryPages() const noexcept {
    return WasmMemoryPages.load(std::memory_order_relaxed);
  }

  /// Getter of peak Wasm memory pages.
  uint64_t getMemoryPeakPages() const noexcept {
    return WasmMemoryPeakPages.load(std::memory_order_relaxed);
  }

  /// Getter of current Wasm memory in bytes.
  uint64_t getMemoryBytes() const noexcept {
    return getMemoryPages() * UINT64_C(65536);
  }

  /// Getter of peak Wasm memory in bytes.
  uint64_t getMemoryPeakBytes() const noexcept {
    return getMemoryPeakPages() * UINT64_C(65536);
  }

  void dumpToLog(const Configure &Conf) const noexcept {
    using namespace std::literals;
    auto Nano = [](auto &&Duration) {
      return std::chrono::nanoseconds(Duration).count();
    };
    const auto &StatConf = Conf.getStatisticsConfigure();
    bool HasAny = StatConf.isTimeMeasuring() ||
                  StatConf.isInstructionCounting() ||
                  StatConf.isCostMeasuring() ||
                  StatConf.isColdStartMeasuring() ||
                  StatConf.isCpuMeasuring() || StatConf.isMemoryMeasuring();
    if (HasAny) {
      spdlog::info("====================  Statistics  ===================="sv);
    }
    if (StatConf.isTimeMeasuring()) {
      spdlog::info(" Total execution time: {} ns"sv, Nano(getTotalExecTime()));
      spdlog::info(" Wasm instructions execution time: {} ns"sv,
                   Nano(getWasmExecTime()));
      spdlog::info(" Host functions execution time: {} ns"sv,
                   Nano(getHostFuncExecTime()));
    }
    if (StatConf.isInstructionCounting()) {
      spdlog::info(" Executed wasm instructions count: {}"sv, getInstrCount());
    }
    if (StatConf.isCostMeasuring()) {
      spdlog::info(" Gas costs: {}"sv, getTotalCost());
    }
    if (StatConf.isInstructionCounting() && StatConf.isTimeMeasuring()) {
      const double IPS = getInstrPerSecond();
      spdlog::info(" Instructions per second: {}"sv,
                   likely(!std::isnan(IPS))
                       ? static_cast<uint64_t>(IPS)
                       : std::numeric_limits<uint64_t>::max());
    }
    if (StatConf.isColdStartMeasuring()) {
      spdlog::info(" Cold start total time: {} ns"sv,
                   Nano(getColdStartTotalTime()));
      spdlog::info("   Load time: {} ns"sv, Nano(getColdStartLoadTime()));
      spdlog::info("   Validation time: {} ns"sv,
                   Nano(getColdStartValidateTime()));
      spdlog::info("   Instantiation time: {} ns"sv,
                   Nano(getColdStartInstantiateTime()));
    }
    if (StatConf.isCpuMeasuring()) {
      spdlog::info(" Process CPU time: {} ns"sv, getCpuTimeNs());
    }
    if (StatConf.isMemoryMeasuring()) {
      spdlog::info(" Wasm linear memory: {} pages ({} bytes)"sv,
                   getMemoryPages(), getMemoryBytes());
      spdlog::info(" Wasm linear memory peak: {} pages ({} bytes)"sv,
                   getMemoryPeakPages(), getMemoryPeakBytes());
    }
    if (HasAny) {
      spdlog::info("=======================   End   ======================"sv);
    }
  }

private:
  std::vector<uint64_t> CostTab;
  std::atomic_uint64_t InstrCnt;
  uint64_t CostLimit;
  std::atomic_uint64_t CostSum;
  Timer::Timer TimeRecorder;
  std::atomic_uint64_t CpuTimeStartNs{0};
  std::atomic_uint64_t CpuTimeTotalNs{0};
  std::atomic_uint64_t WasmMemoryPages{0};
  std::atomic_uint64_t WasmMemoryPeakPages{0};
};

} // namespace Statistics
} // namespace WasmEdge
