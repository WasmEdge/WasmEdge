// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetCallDataSize : public EEI<EEIGetCallDataSize> {
public:
  EEIGetCallDataSize(VM::EVMEnvironment &HostEnv)
      : EEI(HostEnv, "getCallDataSize", 2) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t &Ret);
};

} // namespace Executor
} // namespace SSVM
