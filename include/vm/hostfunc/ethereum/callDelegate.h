// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEICallDelegate : public EEI<EEICallDelegate> {
public:
  EEICallDelegate(VM::EVMEnvironment &HostEnv)
      : EEI(HostEnv, "callDelegate", 700) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst);
};

} // namespace Executor
} // namespace SSVM
