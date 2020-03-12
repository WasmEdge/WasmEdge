// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEISelfDestruct : public EEI<EEISelfDestruct> {
public:
  EEISelfDestruct(VM::EVMEnvironment &HostEnv)
      : EEI(HostEnv, "selfDestruct", 5000) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t AddressOffset);
};

} // namespace Executor
} // namespace SSVM
