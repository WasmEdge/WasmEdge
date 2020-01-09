// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetBlockTimestamp : public EEI<EEIGetBlockTimestamp> {
public:
  EEIGetBlockTimestamp(VM::EVMEnvironment &HostEnv, const uint64_t &Cost = 2)
      : EEI(HostEnv, Cost) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst);
};

} // namespace Executor
} // namespace SSVM
