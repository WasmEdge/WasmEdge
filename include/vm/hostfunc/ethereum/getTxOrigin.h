// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetTxOrigin : public EEI<EEIGetTxOrigin> {
public:
  EEIGetTxOrigin(VM::EVMEnvironment &HostEnv)
      : EEI(HostEnv, "getTxOrigin", 2) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t ResultOffset);
};

} // namespace Executor
} // namespace SSVM
