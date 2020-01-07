// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIUseGas : public EEI<EEIUseGas> {
public:
  EEIUseGas(VM::EVMEnvironment &HostEnv, const uint64_t &Cost = 0)
      : EEI(HostEnv, Cost) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint64_t Amount);
};

} // namespace Executor
} // namespace SSVM
