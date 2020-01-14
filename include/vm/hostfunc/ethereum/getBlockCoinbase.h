// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetBlockCoinbase : public EEI<EEIGetBlockCoinbase> {
public:
  EEIGetBlockCoinbase(VM::EVMEnvironment &HostEnv)
      : EEI(HostEnv, "getBlockCoinbase", 2) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t ResultOffset);
};

} // namespace Executor
} // namespace SSVM
