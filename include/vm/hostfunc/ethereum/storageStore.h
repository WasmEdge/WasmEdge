// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIStorageStore : public EEI<EEIStorageStore> {
public:
  EEIStorageStore(VM::EVMEnvironment &HostEnv)
      : EEI(HostEnv, "storageStore", 5000) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t PathOffset,
               uint32_t ValueOffset);
};

} // namespace Executor
} // namespace SSVM
