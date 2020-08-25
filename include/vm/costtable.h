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

#include <unordered_map>
#include <vector>

namespace SSVM {
namespace VM {

class CostTable {
public:
  CostTable() { setCostTable(Configure::VMType::Wasm); };
  ~CostTable() = default;

  /// Add default cost table by types.
  bool setCostTable(const Configure::VMType &Type) {
    switch (Type) {
    case Configure::VMType::Wasm:
    case Configure::VMType::Wasi:
    case Configure::VMType::SSVM_Process:
      Costs[Type] = std::vector<uint64_t>(UINT16_MAX, 1);
      return true;
    default:
      break;
    }
    return false;
  }

  /// Set customized cost table.
  bool setCostTable(const Configure::VMType &Type,
                    const std::vector<uint64_t> &Table) {
    if (Table.size() < UINT16_MAX) {
      return false;
    }
    Costs[Type] = Table;
    return true;
  }

  /// Get cost table reference by types.
  const std::vector<uint64_t> &getCostTable(const Configure::VMType &Type) {
    if (Costs.find(Type) != Costs.end()) {
      return Costs.at(Type);
    }
    return Costs.at(Configure::VMType::Wasm);
  }

private:
  std::unordered_map<Configure::VMType, std::vector<uint64_t>> Costs;
};

} // namespace VM
} // namespace SSVM