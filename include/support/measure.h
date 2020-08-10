// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/ast/instruction.h"
#include "support/span.h"
#include "time.h"

#include <vector>

namespace SSVM {
namespace Support {

class Measurement {
public:
  Measurement(const uint64_t Lim = UINT64_MAX)
      : CostTab(UINT16_MAX, 0ULL), InstrCnt(0), CostLimit(Lim), CostSum(0) {}
  Measurement(Span<const uint64_t> Tab, const uint64_t Lim = UINT64_MAX)
      : CostTab(Tab.begin(), Tab.end()), InstrCnt(0), CostLimit(Lim),
        CostSum(0) {
    if (CostTab.size() < UINT16_MAX) {
      CostTab.resize(UINT16_MAX);
    }
  }
  ~Measurement() = default;

  /// Increament of instruction counter.
  void incInstrCnt() { ++InstrCnt; }

  /// Getter of instruction counter.
  uint64_t getInstrCnt() const { return InstrCnt; }

  /// Setter of cost table.
  void setCostTable(Span<const uint64_t> NewTable) {
    CostTab.assign(NewTable.begin(), NewTable.end());
    if (CostTab.size() < UINT16_MAX) {
      CostTab.resize(UINT16_MAX);
    }
  }

  /// Adder for instruction costs.
  bool addInstrCost(OpCode Code) { return addCost(CostTab[uint16_t(Code)]); }

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
