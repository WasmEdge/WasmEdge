// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEISelfDestruct : public EEI<EEISelfDestruct> {
public:
  EEISelfDestruct(VM::EVMEnvironment &HostEnv, const uint64_t &Cost = 5000)
      : EEI(HostEnv, Cost) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst);
};

} // namespace Executor
} // namespace SSVM
