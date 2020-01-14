// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIReturnDataCopy : public EEI<EEIReturnDataCopy> {
public:
  EEIReturnDataCopy(VM::EVMEnvironment &HostEnv)
      : EEI(HostEnv, "returnDataCopy", 3) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t ResultOffset,
               uint32_t DataOffset, uint32_t Length);
};

} // namespace Executor
} // namespace SSVM
