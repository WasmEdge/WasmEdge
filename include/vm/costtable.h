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
  CostTable() = default;
  ~CostTable() = default;

  /// Add default cost table by types.
  bool setCostTable(const Configure::VMType &Type) {
    switch (Type) {
    case Configure::VMType::Wasm:
      /// Wasm cost table
      Costs[Type] = std::vector<uint64_t>(256, 1);
      return true;
    case Configure::VMType::Wasi:
      /// Wasi cost table
      Costs[Type] = std::vector<uint64_t>(256, 1);
      return true;
    default:
      break;
    }
    return false;
  }

  /// Set customized cost table.
  bool setCostTable(const Configure::VMType &Type,
                    const std::vector<uint64_t> &Table) {
    if (Table.size() < 0xFF) {
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