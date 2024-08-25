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
    const auto Limit = CostLimit;
    uint64_t OldCostSum = CostSum.load(std::memory_order_relaxed);
    uint64_t NewCostSum;
    do {
      NewCostSum = OldCostSum + Cost;
      if (unlikely(NewCostSum > Limit)) {
        spdlog::error("Cost exceeded limit. Force terminate the execution.");
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

  void dumpToLog(const Configure &Conf) const noexcept {
    auto Nano = [](auto &&Duration) {
      return std::chrono::nanoseconds(Duration).count();
    };
    const auto &StatConf = Conf.getStatisticsConfigure();
    if (StatConf.isTimeMeasuring() || StatConf.isInstructionCounting() ||
        StatConf.isCostMeasuring()) {
      spdlog::info("====================  Statistics  ====================");
    }
    if (StatConf.isTimeMeasuring()) {
      spdlog::info(" Total execution time: {} ns", Nano(getTotalExecTime()));
      spdlog::info(" Wasm instructions execution time: {} ns",
                   Nano(getWasmExecTime()));
      spdlog::info(" Host functions execution time: {} ns",
                   Nano(getHostFuncExecTime()));
    }
    if (StatConf.isInstructionCounting()) {
      spdlog::info(" Executed wasm instructions count: {}", getInstrCount());
    }
    if (StatConf.isCostMeasuring()) {
      spdlog::info(" Gas costs: {}", getTotalCost());
    }
    if (StatConf.isInstructionCounting() && StatConf.isTimeMeasuring()) {
      spdlog::info(" Instructions per second: {}",
                   static_cast<uint64_t>(getInstrPerSecond()));
    }
    if (StatConf.isTimeMeasuring() || StatConf.isInstructionCounting() ||
        StatConf.isCostMeasuring()) {
      spdlog::info("=======================   End   ======================");
    }
  }

private:
  std::vector<uint64_t> CostTab;
  std::atomic_uint64_t InstrCnt;
  uint64_t CostLimit;
  std::atomic_uint64_t CostSum;
  Timer::Timer TimeRecorder;
};

} // namespace Statistics
} // namespace WasmEdge
