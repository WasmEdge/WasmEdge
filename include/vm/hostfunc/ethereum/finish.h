// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIFinish : public EEI<EEIFinish> {
public:
  EEIFinish(VM::EVMEnvironment &HostEnv) : EEI(HostEnv, "finish", 0) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t DataOffset,
               uint32_t DataLength);
};

} // namespace Executor
} // namespace SSVM
