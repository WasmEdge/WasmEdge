// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIUseGas : public EEI<EEIUseGas> {
public:
  EEIUseGas(VM::EVMEnvironment &HostEnv) : EEI(HostEnv, "useGas", 0) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint64_t Amount);
};

} // namespace Executor
} // namespace SSVM
