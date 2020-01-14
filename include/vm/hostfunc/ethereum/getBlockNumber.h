// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetBlockNumber : public EEI<EEIGetBlockNumber> {
public:
  EEIGetBlockNumber(VM::EVMEnvironment &HostEnv)
      : EEI(HostEnv, "getBlockNumber", 2) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint64_t &BlockNumber);
};

} // namespace Executor
} // namespace SSVM
