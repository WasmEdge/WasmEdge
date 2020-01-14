// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetBlockHash : public EEI<EEIGetBlockHash> {
public:
  EEIGetBlockHash(VM::EVMEnvironment &HostEnv)
      : EEI(HostEnv, "getBlockHash", 800) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t &Ret,
               uint64_t Number, uint32_t ResultOffset);
};

} // namespace Executor
} // namespace SSVM
