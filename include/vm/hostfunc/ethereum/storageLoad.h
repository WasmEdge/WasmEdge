// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIStorageLoad : public EEI<EEIStorageLoad> {
public:
  EEIStorageLoad(VM::EVMEnvironment &HostEnv)
      : EEI(HostEnv, "storageLoad", 200) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t PathOffset,
               uint32_t ValueOffset);
};

} // namespace Executor
} // namespace SSVM
