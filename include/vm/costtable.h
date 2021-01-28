// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/vm/costtable.h - cost table class definition -----------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents the definition of cost table of instructions.s
///
//===----------------------------------------------------------------------===//
#pragma once

#include <array>
#include <vector>

namespace SSVM {
namespace VM {

class CostTable {
public:
  CostTable() { Costs.assign(UINT16_MAX + 1, 1); };
  ~CostTable() = default;

  /// Set customized cost table.
  void setCostTable(Span<const uint64_t> Table) {
    Costs.assign(Table.begin(), Table.end());
    if (Costs.size() <= UINT16_MAX) {
      Costs.resize(UINT16_MAX + 1, 0ULL);
    }
  }

  /// Get cost table array.
  Span<const uint64_t> getCostTable() { return Costs; }

private:
  std::vector<uint64_t> Costs;
};

} // namespace VM
} // namespace SSVM
