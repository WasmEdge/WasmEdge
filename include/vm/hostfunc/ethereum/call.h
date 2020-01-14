// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEICall : public EEI<EEICall> {
public:
  EEICall(VM::EVMEnvironment &HostEnv) : EEI(HostEnv, "call", 700) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst);
};

} // namespace Executor
} // namespace SSVM
