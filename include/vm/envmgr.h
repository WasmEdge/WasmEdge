// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/vm/envmgr.h - environment manager class ----------------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents the definition of environment manager class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "configure.h"
#include "costtable.h"
#include "environment.h"

#include <unordered_map>

namespace SSVM {
namespace VM {

class EnvironmentManager {
public:
  EnvironmentManager() = delete;
  EnvironmentManager(Configure &InputConfig)
      : Config(InputConfig), CostTab(), CostLimit(0) {
    CostTab.setCostTable(Configure::VMType::Wasm);
    if (Config.hasVMType(Configure::VMType::Ewasm)) {
      EnvTable[Configure::VMType::Ewasm] = std::make_unique<EVMEnvironment>();
      CostTab.setCostTable(Configure::VMType::Ewasm);
    }
    if (Config.hasVMType(Configure::VMType::Wasi)) {
      EnvTable[Configure::VMType::Wasi] = std::make_unique<WasiEnvironment>();
      CostTab.setCostTable(Configure::VMType::Wasi);
    }
  }
  ~EnvironmentManager() = default;

  Configure &getConfigure() const { return Config; }

  const std::vector<int> &getCostTable() {
    if (Config.hasVMType(Configure::VMType::Ewasm)) {
      return CostTab.getCostTable(Configure::VMType::Ewasm);
    } else if (Config.hasVMType(Configure::VMType::Wasi)) {
      return CostTab.getCostTable(Configure::VMType::Wasi);
    }
    return CostTab.getCostTable(Configure::VMType::Wasm);
  }

  /// Getter of Environment.
  template <typename T> TypeEnv<T> *getEnvironment(Configure::VMType Type) {
    if (EnvTable.find(Type) != EnvTable.end()) {
      return dynamic_cast<T *>(EnvTable[Type].get());
    }
    return nullptr;
  }

  /// Setter of cost limit.
  void setCostLimit(const int &Limit) { CostLimit = Limit; }

  /// Getter of cost limit.
  int getCostLimit() { return CostLimit; }

private:
  std::unordered_map<Configure::VMType, std::unique_ptr<Environment>> EnvTable;
  CostTable CostTab;
  int CostLimit;
  Configure &Config;
};

} // namespace VM
} // namespace SSVM