// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetTxGasPrice : public EEI<EEIGetTxGasPrice> {
public:
  EEIGetTxGasPrice(VM::EVMEnvironment &HostEnv, const uint64_t &Cost = 2)
      : EEI(HostEnv, Cost) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint64_t ResultOffset);
};

} // namespace Executor
} // namespace SSVM
