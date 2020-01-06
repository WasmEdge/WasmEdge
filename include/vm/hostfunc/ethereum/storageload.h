// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIStorageLoad : public EEI {
public:
  EEIStorageLoad(VM::EVMEnvironment &Env, uint64_t Cost = 100);

  ErrCode run(VM::EnvironmentManager &EnvMgr, StackManager &StackMgr,
              Instance::MemoryInstance &MemInst) override;

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t PathOffset,
               uint32_t ValueOffset);
};

} // namespace Executor
} // namespace SSVM
