// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEICreate : public EEI<EEICreate> {
public:
  EEICreate(VM::EVMEnvironment &HostEnv) : EEI(HostEnv, "create", 32000) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst);
};

} // namespace Executor
} // namespace SSVM
