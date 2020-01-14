// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEICallCode : public EEI<EEICallCode> {
public:
  EEICallCode(VM::EVMEnvironment &HostEnv) : EEI(HostEnv, "callCode", 700) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst);
};

} // namespace Executor
} // namespace SSVM
