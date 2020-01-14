// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetCaller : public EEI<EEIGetCaller> {
public:
  EEIGetCaller(VM::EVMEnvironment &HostEnv) : EEI(HostEnv, "getCaller", 2) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t ResultOffset);
};

} // namespace Executor
} // namespace SSVM
