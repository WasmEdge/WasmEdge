// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEICallCode : public EEI<EEICallCode> {
public:
  EEICallCode(VM::EVMEnvironment &HostEnv) : EEI(HostEnv, "callCode", 700) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t &Ret, uint64_t Gas,
               uint32_t AddressOffset, uint32_t ValueOffset,
               uint32_t DataOffset, uint32_t DataLength);
};

} // namespace Executor
} // namespace SSVM
