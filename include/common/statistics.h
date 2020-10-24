// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/common/statistics.h - Interpreter statistics definition ------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the statistics class of interpreter's runtime.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/astdef.h"
#include "common/errcode.h"
#include "span.h"
#include "timer.h"

#include <vector>

namespace SSVM {
namespace Statistics {

class Statistics {
public:
  Statistics(const uint64_t Lim = UINT64_MAX)
      : CostTab(UINT16_MAX + 1, 0ULL), InstrCnt(0), CostLimit(Lim), CostSum(0) {
  }
  Statistics(Span<const uint64_t> Tab, const uint64_t Lim = UINT64_MAX)
      : CostTab(Tab.begin(), Tab.end()), InstrCnt(0), CostLimit(Lim),
        CostSum(0) {
    if (CostTab.size() < UINT16_MAX + 1) {
      CostTab.resize(UINT16_MAX + 1);
    }
  }
  ~Statistics() = default;

  /// Increment of instruction counter.
  void incInstrCount() { ++InstrCnt; }

  /// Getter of instruction counter.
  uint64_t getInstrCount() const { return InstrCnt; }
  uint64_t &getInstrCountRef() { return InstrCnt; }

  /// Getter of instruction per second.
  double getInstrPerSecond() const {
    return InstrCnt / std::chrono::duration<double>(getWasmExecTime()).count();
  }

  /// Setter and setter of cost table.
  void setCostTable(Span<const uint64_t> NewTable) {
    CostTab.assign(NewTable.begin(), NewTable.end());
    if (unlikely(CostTab.size() < UINT16_MAX + 1)) {
      CostTab.resize(UINT16_MAX + 1);
    }
  }
  Span<const uint64_t> getCostTable() const noexcept { return CostTab; }
  Span<uint64_t> getCostTable() noexcept { return CostTab; }

  /// Adder of instruction costs.
  bool addInstrCost(OpCode Code) { return addCost(CostTab[uint16_t(Code)]); }

  /// Getter of total gas cost.
  uint64_t getTotalCost() const { return CostSum; }
  uint64_t &getTotalCostRef() { return CostSum; }

  /// Getter and setter of cost limit.
  void setCostLimit(uint64_t Lim) { CostLimit = Lim; }
  uint64_t getCostLimit() const { return CostLimit; }

  /// Add cost and return false if exceeded limit.
  bool addCost(const uint64_t &Cost) {
    CostSum += Cost;
    if (unlikely(CostSum > CostLimit)) {
      CostSum = CostLimit;
      return false;
    }
    return true;
  }

  /// Return cost back.
  bool subCost(const uint64_t &Cost) {
    if (likely(CostSum > Cost)) {
      CostSum -= Cost;
      return true;
    }
    CostSum = 0;
    return false;
  }

  /// Clear measurement data for instructions.
  void clear() {
    TimeRecorder.reset();
    InstrCnt = 0;
    CostSum = 0;
  }

  /// Start recording wasm time.
  void startRecordWasm() { TimeRecorder.startRecord(Timer::TimerTag::Wasm); }

  /// Stop recording wasm time.
  void stopRecordWasm() { TimeRecorder.stopRecord(Timer::TimerTag::Wasm); }

  /// Start recording host function time.
  void startRecordHost() {
    TimeRecorder.startRecord(Timer::TimerTag::HostFunc);
  }

  /// Stop recording host function time.
  void stopRecordHost() { TimeRecorder.stopRecord(Timer::TimerTag::HostFunc); }

  /// Getter of execution time.
  Timer::Timer::Clock::duration getWasmExecTime() const {
    return TimeRecorder.getRecord(Timer::TimerTag::Wasm);
  }
  Timer::Timer::Clock::duration getHostFuncExecTime() const {
    return TimeRecorder.getRecord(Timer::TimerTag::HostFunc);
  }
  Timer::Timer::Clock::duration getTotalExecTime() const {
    return TimeRecorder.getRecord(Timer::TimerTag::Wasm) +
           TimeRecorder.getRecord(Timer::TimerTag::HostFunc);
  }

private:
  std::vector<uint64_t> CostTab;
  uint64_t InstrCnt;
  uint64_t CostLimit;
  uint64_t CostSum;
  Timer::Timer TimeRecorder;
};

} // namespace Statistics
} // namespace SSVM
