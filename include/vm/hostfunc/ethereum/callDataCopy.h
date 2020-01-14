// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEICallDataCopy : public EEI<EEICallDataCopy> {
public:
  EEICallDataCopy(VM::EVMEnvironment &HostEnv)
      : EEI(HostEnv, "callDataCopy", 3) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t ResultOffset,
               uint32_t DataOffset, uint32_t Length);
};

} // namespace Executor
} // namespace SSVM
