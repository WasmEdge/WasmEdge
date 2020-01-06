// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetCallDataSize : public EEI {
public:
  EEIGetCallDataSize(VM::EVMEnvironment &Env, uint64_t Cost = 100);

  ErrCode run(VM::EnvironmentManager &EnvMgr, StackManager &StackMgr,
              Instance::MemoryInstance &MemInst) override;

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t &Ret);
};

} // namespace Executor
} // namespace SSVM
