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

#include "configure.h"

#include <array>
#include <vector>

namespace SSVM {
namespace VM {

class CostTable {
public:
  CostTable() { setCostTable(Configure::VMType::Wasm); };
  ~CostTable() = default;

  /// Add default cost table by types.
  bool setCostTable(const Configure::VMType &Type) {
    if (Type >= Configure::VMType::Max) {
      return false;
    }
    Costs[uint8_t(Type)].assign(UINT16_MAX + 1, 1);
    return true;
  }

  /// Set customized cost table.
  bool setCostTable(Configure::VMType Type, std::vector<uint64_t> Table) {
    if (Table.size() <= UINT16_MAX) {
      return false;
    }
    if (Type >= Configure::VMType::Max) {
      Type = Configure::VMType::Wasm;
    }
    Costs[uint8_t(Type)] = std::move(Table);
    return true;
  }

  /// Get cost table reference by types.
  const std::vector<uint64_t> &getCostTable(Configure::VMType Type) {
    if (Type >= Configure::VMType::Max) {
      Type = Configure::VMType::Wasm;
    }
    return Costs[uint8_t(Type)];
  }

private:
  std::array<std::vector<uint64_t>, uint8_t(Configure::VMType::Max)> Costs;
};

} // namespace VM
} // namespace SSVM
