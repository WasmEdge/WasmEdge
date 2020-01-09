// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetExternalCodeSize : public EEI<EEIGetExternalCodeSize> {
public:
  EEIGetExternalCodeSize(VM::EVMEnvironment &HostEnv,
                         const uint64_t &Cost = 700)
      : EEI(HostEnv, Cost) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst);
};

} // namespace Executor
} // namespace SSVM
