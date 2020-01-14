// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetExternalCodeSize : public EEI<EEIGetExternalCodeSize> {
public:
  EEIGetExternalCodeSize(VM::EVMEnvironment &HostEnv)
      : EEI(HostEnv, "getExternalCodeSize", 700) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t &Ret,
               uint32_t AddressOffset);
};

} // namespace Executor
} // namespace SSVM
