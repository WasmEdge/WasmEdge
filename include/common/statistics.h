// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/common/statistics.h - Interpreter statistics definition ------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of statistics information of
/// interpreteer's runtime.
///
//===----------------------------------------------------------------------===//
#pragma once

namespace SSVM {
namespace Statistics {
class Statistics {
public:
  Statistics() = default;
  ~Statistics() = default;
  void setWasmExecTime(uint64_t WET) { WasmExecTime_ = WET; }
  uint64_t getWasmExecTime() const { return WasmExecTime_; }
  void setHostFuncExecTime(uint64_t HFET) { HostFuncExecTime_ = HFET; }
  uint64_t getHostFuncExecTime() const { return HostFuncExecTime_; }
  void setInstrCount(uint64_t IC) { InstrCount_ = IC; }
  uint64_t getInstrCount() const { return InstrCount_; }
  void setTotalGasCost(uint64_t TGC) { TotalGasCost_ = TGC; }
  uint64_t getTotalGasCost() const { return TotalGasCost_; }
  uint64_t getTotalExecTime() const {
    return WasmExecTime_ + HostFuncExecTime_;
  }
  double getInstrPerSecond() const {
    return InstrCount_ * 1000000.0 / WasmExecTime_;
  }

private:
  uint64_t WasmExecTime_;
  uint64_t HostFuncExecTime_;
  uint64_t InstrCount_;
  uint64_t TotalGasCost_;
};
} // namespace Statistics
} // namespace SSVM
