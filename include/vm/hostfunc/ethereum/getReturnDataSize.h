// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetReturnDataSize : public EEI<EEIGetReturnDataSize> {
public:
  EEIGetReturnDataSize(VM::EVMEnvironment &HostEnv, const uint64_t &Cost = 2)
      : EEI(HostEnv, Cost) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t &DataSize);
};

} // namespace Executor
} // namespace SSVM
