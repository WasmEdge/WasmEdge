// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetCodeSize : public EEI<EEIGetCodeSize> {
public:
  EEIGetCodeSize(VM::EVMEnvironment &HostEnv)
      : EEI(HostEnv, "getCodeSize", 2) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t &Ret);
};

} // namespace Executor
} // namespace SSVM
