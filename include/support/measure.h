// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/ast/instruction.h"
#include "time.h"

#include <vector>

namespace SSVM {
namespace Support {

class Measurement {
public:
  Measurement(const uint64_t Lim = UINT64_MAX)
      : CostTab(256, 0ULL), InstrCnt(0), CostLimit(Lim), CostSum(0) {}
  Measurement(const std::vector<uint64_t> &Tab, const uint64_t Lim = UINT64_MAX)
      : CostTab(Tab), InstrCnt(0), CostLimit(Lim), CostSum(0) {
    if (CostTab.size() < 256) {
      CostTab.resize(256);
    }
  }
  ~Measurement() = default;

  /// Increament of instruction counter.
  void incInstrCnt() { ++InstrCnt; }

  /// Getter of instruction counter.
  const uint64_t getInstrCnt() const { return InstrCnt; }

  /// Setter of cost table.
  void setCostTable(const std::vector<uint64_t> &NewTable) {
    CostTab = NewTable;
    if (CostTab.size() < 256) {
      CostTab.resize(256);
    }
  }

  /// Adder for instruction costs.
  bool addInstrCost(const AST::Instruction::OpCode &Code) {
    return addCost(CostTab[static_cast<uint64_t>(Code)]);
  }

  /// Getter reference of cost limit.
  uint64_t &getCostLimit() { return CostLimit; }

  /// Getter reference of cost sum.
  uint64_t &getCostSum() { return CostSum; }

  /// Add cost and return false if exceeded limit.
  bool addCost(const uint64_t &Cost) {
    CostSum += Cost;
    if (CostSum > CostLimit) {
      CostSum = CostLimit;
      return false;
    }
    return true;
  }

  /// Return cost back.
  bool subCost(const uint64_t &Cost) {
    if (CostSum > Cost) {
      CostSum -= Cost;
      return true;
    }
    CostSum = 0;
    return false;
  }

  /// Getter of time recorder.
  Support::TimeRecord &getTimeRecorder() { return TimeRecorder; }

  /// Clear measurement data for instructions.
  void clear() {
    TimeRecorder.reset();
    InstrCnt = 0;
  }

private:
  Support::TimeRecord TimeRecorder;
  std::vector<uint64_t> CostTab;
  uint64_t InstrCnt;
  uint64_t CostLimit;
  uint64_t CostSum;
  /// TODO: Can add a instruction statistics for OpCodes.
};

} // namespace Support
} // namespace SSVM