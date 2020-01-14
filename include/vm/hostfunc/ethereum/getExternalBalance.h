// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetExternalBalance : public EEI<EEIGetExternalBalance> {
public:
  EEIGetExternalBalance(VM::EVMEnvironment &HostEnv)
      : EEI(HostEnv, "getExternalBalance", 400) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t AddressOffset,
               uint32_t ResultOffset);
};

} // namespace Executor
} // namespace SSVM
