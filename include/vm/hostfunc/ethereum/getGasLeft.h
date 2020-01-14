// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetGasLeft : public EEI<EEIGetGasLeft> {
public:
  EEIGetGasLeft(VM::EVMEnvironment &HostEnv) : EEI(HostEnv, "getGasLeft", 2) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint64_t &GasLeft);
};

} // namespace Executor
} // namespace SSVM
