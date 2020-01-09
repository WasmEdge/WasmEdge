// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIRevert : public EEI<EEIRevert> {
public:
  EEIRevert(VM::EVMEnvironment &HostEnv, const uint64_t &Cost = 0)
      : EEI(HostEnv, Cost) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t DataOffset,
               uint32_t DataLength);
};

} // namespace Executor
} // namespace SSVM
