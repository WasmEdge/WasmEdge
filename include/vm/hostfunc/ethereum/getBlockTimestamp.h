// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetBlockTimestamp : public EEI<EEIGetBlockTimestamp> {
public:
  EEIGetBlockTimestamp(VM::EVMEnvironment &HostEnv)
      : EEI(HostEnv, "getBlockTimestamp", 2) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint64_t &BlockTimestamp);
};

} // namespace Executor
} // namespace SSVM
