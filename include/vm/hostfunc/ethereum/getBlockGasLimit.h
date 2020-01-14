// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetBlockGasLimit : public EEI<EEIGetBlockGasLimit> {
public:
  EEIGetBlockGasLimit(VM::EVMEnvironment &HostEnv)
      : EEI(HostEnv, "getBlockGasLimit", 2) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint64_t &GasLimit);
};

} // namespace Executor
} // namespace SSVM
