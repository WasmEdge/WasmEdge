// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEICodeCopy : public EEI<EEICodeCopy> {
public:
  EEICodeCopy(VM::EVMEnvironment &HostEnv)
      : EEI(HostEnv, "codeCopy", 3) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t ResultOffset,
               uint32_t CodeOffset, uint32_t Length);
};

} // namespace Executor
} // namespace SSVM
