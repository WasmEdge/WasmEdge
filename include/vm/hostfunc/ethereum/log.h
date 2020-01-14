// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEILog : public EEI<EEILog> {
public:
  EEILog(VM::EVMEnvironment &HostEnv) : EEI(HostEnv, "log", 375) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t DataOffset,
               uint32_t DataLength, uint32_t NumberOfTopics, uint32_t Topic1,
               uint32_t Topic2, uint32_t Topic3, uint32_t Topic4);
};

} // namespace Executor
} // namespace SSVM
