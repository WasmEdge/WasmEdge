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
    case Configure::VMType::Ewasm:
      Costs[Type] = std::vector<uint64_t>(256, 0);
      /// TODO: Ewasm cost table
      /*
      Costs[Type] = std::vector<uint64_t>{
          /// 0x00 - 0x0F
          /// Note: Due to the instructions are only if and else,
          /// cost of if-end should be Costs[0x05] (i.e. if),
          /// cost of if-then should be Costs[if] + Costs[else] (i.e. if +
          /// else),
          /// cost of if-else should be Costs[if] + Costs[else] (i.e. if + else)
          1, 1, 1, 1, 1, 90, 0, 0, 0, 0, 0, 0, 90, 90, 120, 90,
          /// 0x10 - 0x1F
          90, 10000, 0, 0, 0, 0, 0, 0, 0, 0, 120, 120, 0, 0, 0, 0,
          /// 0x20 - 0x2F
          120, 120, 120, 120, 120, 0, 0, 0, 120, 120, 120, 120, 120, 120, 120,
          120,
          /// 0x30 - 0x3F
          120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120,
          120, 100,
          /// 0x40 - 0x4F
          10000, 1, 1, 1, 1, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
          /// 0x50 - 0x5F
          45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
          /// 0x60 - 0x6F
          45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 36000, 36000,
          36000,
          /// 0x70 - 0x7F
          36000, 45, 45, 45, 67, 67, 67, 90, 90, 45, 45, 45, 45, 45, 45, 36000,
          /// 0x80 - 0x8F
          36000, 36000, 36000, 45, 45, 45, 67, 67, 67, 90, 90, 0, 0, 0, 0, 0,
          /// 0x90 - 0x9F
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
          /// 0xA0 - 0xAF
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
          /// 0xB0 - 0xBF
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
          /// 0xC0 - 0xCF
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
          /// 0xD0 - 0xDF
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
          /// 0xE0 - 0xEF
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
          /// 0xF0 - 0xFF
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      */
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