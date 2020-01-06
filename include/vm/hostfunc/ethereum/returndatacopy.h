// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIReturnDataCopy : public EEI {
public:
  EEIReturnDataCopy(VM::EVMEnvironment &Env, uint64_t Cost = 100);

  ErrCode run(VM::EnvironmentManager &EnvMgr, StackManager &StackMgr,
              Instance::MemoryInstance &MemInst) override;

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t ResultOffset,
               uint32_t DataOffset, uint32_t Length);
};

} // namespace Executor
} // namespace SSVM
