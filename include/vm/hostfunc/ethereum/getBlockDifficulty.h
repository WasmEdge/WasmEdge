// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetBlockDifficulty : public EEI<EEIGetBlockDifficulty> {
public:
  EEIGetBlockDifficulty(VM::EVMEnvironment &HostEnv)
      : EEI(HostEnv, "getBlockDifficulty", 2) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t ResultOffset);
};

} // namespace Executor
} // namespace SSVM
